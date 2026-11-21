#include "Rate_24_Backend.h"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QCoreApplication>
#include <QDate>
#include <QSet>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>
#include <QDebug>
#include <QtGlobal>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>
#include <vector>


namespace {
constexpr int kMaxQuality  = 100;
constexpr int kMaxMinutes  = 24 * 60;
constexpr int kDayStartHour = 4;      // η "ημέρα" αλλάζει στις 04:00 (π.χ. 03:59 ανήκει ακόμα στην προηγούμενη ημερομηνία)
constexpr int kMaxFillDays = 365;     // πόσες μέρες πίσω συμπληρώνονται αυτόματα με μηδενικά

// "Τρέχουσα" ώρα μετατοπισμένη κατά 4 ώρες: ό,τι γίνεται πριν τις 04:00 μετράει για την προηγούμενη μέρα
QDateTime effectiveNow() {
    return QDateTime::currentDateTime().addSecs(-kDayStartHour * 3600);
}

QString todayString() {
    return effectiveNow().toString("yyyy-MM-dd");
}

QString dataDir() {
    static const QString dir = [] {
        // Καρφωτό μονοπάτι: /home/Vasilis/Documents/Rate24
        QString d = QDir::homePath() + "/Documents/Rate24";

        QDir().mkpath(d); // Δημιουργεί τον φάκελο αυτόματα αν δεν υπάρχει
        return d;
    }();
    return dir;
}

QString historyFilePath() {
    static const QString path = [] {
        const QString p = QDir(dataDir()).filePath("performance.json");
        // Μεταφορά (αντιγραφή) του παλιού αρχείου από τον τρέχοντα φάκελο, μία φορά
        if (!QFile::exists(p) && QFile::exists("performance.json")) {
            QFile::copy("performance.json", p);
            qDebug() << "Migrated existing performance.json to" << p;
        }
        qDebug() << "History file:" << p;
        return p;
    }();
    return path;
}

QString draftFilePath() { // προσωρινό αρχείο: σβήνεται στο saveDay
    return QDir(dataDir()).filePath("draft.json");
}

// ---------- Κοινός υπολογισμός σκορ (ΙΔΙΟΣ αλγόριθμος για κανονική εκτέλεση, vacation mode και επεξεργασία ημέρας) ----------

struct ScoreIn {
    int realTime;
    int minTime;
    int quality;
    int minQuality;
    double importance;   // N − priority, με N = ΣΥΝΟΛΙΚΟΣ αριθμός δραστηριοτήτων (ανεξάρτητα από το vacation mode)
    bool sd;
    bool included;       // false = δραστηριότητα εκτός αξιολόγησης (vacation mode)
};

// worths: συνεισφορά κάθε δραστηριότητας στο σκορ ΤΗΣ ΗΜΕΡΑΣ (κανονικοποιημένο στις δραστηριότητες που αξιολογούνται)
// shares: συνεισφορά στην κλίμακα της ΠΛΗΡΟΥΣ ημέρας (σταθερή μονάδα σύγκρισης για διαγράμματα/ρεκόρ ανά δραστηριότητα)
double computeScores(const std::vector<ScoreIn>& in, std::vector<double>& worths, std::vector<double>& shares) {
    worths.assign(in.size(), 0.0);
    shares.assign(in.size(), 0.0);

    double maxPossible = 0.0; // μόνο όσες αξιολογούνται
    double fullMax = 0.0;     // όλες οι δραστηριότητες
    for (const auto& a : in) {
        fullMax += 1.0 * a.importance;
        if (a.included) maxPossible += 1.0 * a.importance;
    }
    if (maxPossible == 0.0) return 0.0;

    double dayScore = 0.0;
    for (size_t i = 0; i < in.size(); ++i) {
        const ScoreIn& a = in[i];
        if (!a.included) continue;

        // Προστασία από διαίρεση με το 0 (min = 0 σημαίνει "δεν υπάρχει απαίτηση")
        // και αν ο χρόνος είναι 0, ούτε η ολοκλήρωση ούτε η ποιότητα μετράνε.
        double completion = 0.0;
        double quality = 0.0;
        if (a.realTime > 0) {
            completion = (a.minTime > 0) ? (double)a.realTime / a.minTime : 1.0;
            quality = (a.minQuality > 0) ? (double)a.quality / a.minQuality : 1.0;
        }
        double ratio = completion * quality;
        double justify = a.sd ? 0.10 * maxPossible : 0.0;

        worths[i] = (((ratio * a.importance) + justify) / maxPossible) * 100.0;
        shares[i] = (fullMax > 0.0 ? (ratio * a.importance / fullMax) * 100.0 : 0.0) + (a.sd ? 10.0 : 0.0);
        dayScore += (ratio * a.importance) + justify;
    }
    return (dayScore / maxPossible) * 100.0;
}

// ---------- Βοηθητικά για το performance.json ----------

// Διαβάζει το ιστορικό. Επιστρέφει false αν το αρχείο υπάρχει αλλά είναι χαλασμένο (γίνεται .bak αντίγραφο).
bool loadHistory(QJsonArray& out) {
    out = QJsonArray();
    QFile f(historyFilePath());
    if (!f.exists()) return true;
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.readAll();
    f.close();
    if (data.trimmed().isEmpty()) return true;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        QFile::remove(historyFilePath() + ".bak");
        QFile::copy(historyFilePath(), historyFilePath() + ".bak");
        qDebug() << "JSON parsing failed. Corrupt file backed up to" << historyFilePath() + ".bak";
        return false;
    }
    out = doc.array();
    return true;
}

void sortByDate(QJsonArray& arr) {
    QList<QJsonValue> list;
    for (const QJsonValue& v : arr) list.append(v);
    std::stable_sort(list.begin(), list.end(), [](const QJsonValue& a, const QJsonValue& b) {
        return a.toObject()["date"].toString() < b.toObject()["date"].toString();
    });
    QJsonArray out;
    for (const QJsonValue& v : list) out.append(v);
    arr = out;
}

// Ατομική εγγραφή (QSaveFile), με ταξινόμηση κατά ημερομηνία
bool saveHistory(QJsonArray arr) {
    sortByDate(arr);
    QSaveFile saveFile(historyFilePath());
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Error: Could not open save file for writing.";
        return false;
    }
    saveFile.write(QJsonDocument(arr).toJson());
    if (!saveFile.commit()) {
        qDebug() << "Error: Commit failed, could not replace file.";
        return false;
    }
    return true;
}

// Δείκτης της ΤΕΛΕΥΤΑΙΑΣ εγγραφής με αυτή την ημερομηνία (-1 αν δεν υπάρχει)
int findByDate(const QJsonArray& arr, const QString& date) {
    for (int i = arr.size() - 1; i >= 0; --i) {
        if (arr[i].toObject()["date"].toString() == date) return i;
    }
    return -1;
}

// Εγγραφή-"placeholder" (μηδενική ή ημιτελής) που μπορεί να αντικατασταθεί από πραγματική καταχώρηση
bool isPlaceholder(const QJsonObject& e) {
    return e["missed"].toBool() || e["incomplete"].toBool();
}

// Αντικαθιστά (και αφαιρεί διπλότυπα της ίδιας ημερομηνίας) ή προσθέτει την εγγραφή
void putEntry(QJsonArray& arr, const QJsonObject& entry) {
    const QString date = entry["date"].toString();
    QJsonArray out;
    for (const QJsonValue& v : arr) {
        if (v.toObject()["date"].toString() != date) out.append(v);
    }
    out.append(entry);
    arr = out;
}

// Χτίζει τα δεδομένα αξιολόγησης για ΟΛΕΣ τις δραστηριότητες (all).
// work = οι δραστηριότητες που ισχύουν σήμερα (όλες, ή το υποσύνολο του vacation mode).
// done = πόσες δραστηριότητες της work έχουν συμπληρωθεί (οι υπόλοιπες μετράνε 0).
// workIndexOfAll[i] = θέση της i-οστής δραστηριότητας μέσα στη work, ή -1 αν είναι εκτός.
template <typename Vec>
std::vector<ScoreIn> buildInputs(const Vec& all, const Vec& work, int done, std::vector<int>& workIndexOfAll) {
    const int n = (int)all.size();
    std::vector<ScoreIn> in;
    in.reserve(n);
    workIndexOfAll.assign(n, -1);
    for (int i = 0; i < n; ++i) {
        const auto& a = all[i];
        int j = -1;
        for (int k = 0; k < (int)work.size(); ++k) {
            if (work[k].get_name() == a.get_name()) { j = k; break; }
        }
        workIndexOfAll[i] = j;
        const bool included = j >= 0;
        const bool filled = included && j < done;
        const int rt = filled ? work[j].get_real_time() : 0;
        const int q = filled ? work[j].get_quality() : 0;
        const bool sd = filled && work[j].get_is_sd();
        in.push_back({ rt, a.get_min_time(), q, a.get_min_quality(), double(n - a.get_priority()), sd, included });
    }
    return in;
}

// Φτιάχνει μια εγγραφή ημέρας. Περιέχει ΠΑΝΤΑ όλες τις δραστηριότητες· όσες είναι εκτός αξιολόγησης
// (vacation mode) έχουν "excluded": true.
template <typename Vec>
QJsonObject makeDayObject(const QString& date, const Vec& all, const Vec& work, int done,
                          const QString& notes, const QStringList& noteList, bool vacation) {
    std::vector<int> wi;
    const std::vector<ScoreIn> in = buildInputs(all, work, done, wi);
    std::vector<double> worths, shares;
    const double dayScore = computeScores(in, worths, shares);

    QJsonObject day;
    day["date"] = date;
    day["score"] = dayScore;
    day["notes"] = notes;
    if (vacation) day["vacation"] = true;

    QJsonArray arr;
    for (int i = 0; i < (int)all.size(); ++i) {
        const auto& a = all[i];
        QJsonObject o;
        o["name"] = a.get_name();
        o["priority"] = a.get_priority();
        o["real_time"] = in[i].realTime;
        o["min_time"] = a.get_min_time();
        o["quality"] = in[i].quality;
        o["min_quality"] = a.get_min_quality();
        o["score"] = worths[i];
        o["share"] = shares[i];
        o["sd_used"] = in[i].sd;
        o["note"] = (wi[i] >= 0 && wi[i] < noteList.size()) ? noteList[wi[i]] : QString();
        if (!in[i].included) o["excluded"] = true;
        arr.append(o);
    }
    day["activities"] = arr;
    return day;
}

// Δημιουργεί το σύνολο δραστηριοτήτων ενός draft από τα ονόματα που περιέχει.
template <typename Vec>
bool buildWorkingSet(const Vec& all, const QJsonArray& arr, bool vacation, Vec& out) {
    out.clear();
    for (const QJsonValue& v : arr) {
        const QString name = v.toObject()["name"].toString();
        bool found = false;
        for (const auto& a : all) {
            if (a.get_name() == name) { out.push_back(a); found = true; break; }
        }
        if (!found) return false;
    }
    if (out.empty()) return false;
    if (!vacation && out.size() != all.size()) return false;
    return true;
}

// ---------- Βοηθητικά για την εξαγωγή σε txt ----------

QString padRight(const QString& s, int w) { return s.left(w).leftJustified(w, ' '); }
QString padLeft(const QString& s, int w)  { return s.right(w).rightJustified(w, ' '); }

// Σπάει κείμενο σε γραμμές το πολύ `width` χαρακτήρων (και πολύ μακριές "λέξεις")
QStringList wrapText(const QString& text, int width) {
    QStringList out;
    const QStringList paragraphs = text.split('\n');
    for (const QString& para : paragraphs) {
        if (para.trimmed().isEmpty()) { out << QString(); continue; }
        QString line;
        const QStringList words = para.split(' ', Qt::SkipEmptyParts);
        for (QString w : words) {
            while (w.size() > width) {
                if (!line.isEmpty()) { out << line; line.clear(); }
                out << w.left(width);
                w = w.mid(width);
            }
            if (line.isEmpty()) line = w;
            else if (line.size() + 1 + w.size() <= width) line += " " + w;
            else { out << line; line = w; }
        }
        if (!line.isEmpty()) out << line;
    }
    while (!out.isEmpty() && out.last().isEmpty()) out.removeLast();
    return out;
}

QString dayTag(const QJsonObject& o) {
    QStringList t;
    if (o["vacation"].toBool()) t << "VACATION";
    if (o["missed"].toBool()) t << "MISSED (no data)";
    else if (o["incomplete"].toBool()) t << "INCOMPLETE";
    if (o["edited"].toBool()) t << "edited";
    return t.isEmpty() ? QString() : QString("  <- ") + t.join(", ");
}

// Παλιές εγγραφές χωρίς "priority": γράφεται η προτεραιότητα του τρέχοντος ορισμού (με βάση το όνομα),
// ώστε αν αλλάξουν αργότερα οι προτεραιότητες, οι παλιές μέρες να μη ξαναυπολογίζονται διαφορετικά.
template <typename Vec>
bool stampPriorities(QJsonArray& history, const Vec& acts) {
    bool changed = false;
    for (int d = 0; d < history.size(); ++d) {
        QJsonObject day = history[d].toObject();
        QJsonArray list = day["activities"].toArray();
        bool dayChanged = false;
        for (int i = 0; i < list.size(); ++i) {
            QJsonObject o = list[i].toObject();
            if (o.contains("priority")) continue;
            const QString name = o["name"].toString();
            for (const auto& a : acts) {
                if (a.get_name() == name) {
                    o["priority"] = a.get_priority();
                    list[i] = o;
                    dayChanged = true;
                    break;
                }
            }
        }
        if (dayChanged) {
            day["activities"] = list;
            history[d] = day;
            changed = true;
        }
    }
    return changed;
}
} // namespace

Rate_24_Backend::Rate_24_Backend(QObject *parent) : QObject(parent) {
    loadDefaultActivities();
    finalizeStaleDraft(); // draft μιας παλιότερης μέρας → γίνεται εγγραφή (ό,τι δεν συμπληρώθηκε = 0)
    fillMissedDays();     // μέρες που δεν άνοιξε καθόλου το πρόγραμμα → εγγραφές με μηδενικά
    loadDraft(); // αν υπάρχει ημιτελής μέρα (ίδια ημερομηνία), συνεχίζουμε από εκεί
}

void Rate_24_Backend::loadDefaultActivities() {
    activities.clear();
    activities.push_back(Activity("Cybersecurity Studying", 60, 65, 1));
    activities.push_back(Activity("University Studying", 30, 65, 2));
    activities.push_back(Activity("Piano Studying", 30, 65, 3));
    activities.push_back(Activity("Book Reading", 30, 65, 4));
    activities.push_back(Activity("Greek Studying", 15, 65, 5));
    activities.push_back(Activity("English Studying", 15, 65, 5));

    allActivities = activities; // ο πλήρης ορισμός (το "activities" μπορεί να περιορίζεται στο vacation mode)
    m_vacation = false;
    currentIndex = 0;
}

int Rate_24_Backend::getActivityCount() { return activities.size(); }

// ---------- Vacation mode ----------

// Πληροφορίες όλων των δραστηριοτήτων για την οθόνη επιλογής
QVariantList Rate_24_Backend::getActivityInfo() {
    QVariantList list;
    for (const auto& a : allActivities) {
        QVariantMap m;
        m["name"] = a.get_name();
        m["min_time"] = a.get_min_time();
        m["min_quality"] = a.get_min_quality();
        m["priority"] = a.get_priority();
        list.append(m);
    }
    return list;
}

// Ενεργοποιεί το vacation mode με τις δραστηριότητες που επιλέχθηκαν. Επιτρέπεται μόνο πριν ξεκινήσει η καταγραφή.
bool Rate_24_Backend::startVacation(const QStringList& names) {
    if (currentIndex != 0) return false;
    decltype(activities) subset;
    for (const auto& a : allActivities) {
        if (names.contains(a.get_name())) subset.push_back(a);
    }
    if (subset.empty()) return false;
    activities = subset;
    currentIndex = 0;
    m_vacation = true;
    return true;
}

// Επιστροφή στο κανονικό mode (όλες οι δραστηριότητες)
void Rate_24_Backend::startNormal() {
    if (currentIndex != 0) return;
    loadDefaultActivities();
}

bool Rate_24_Backend::isVacation() { return m_vacation; }

int Rate_24_Backend::progressCount() { return (int)currentIndex; }

QString Rate_24_Backend::getCurrentActivityName() {
    if (currentIndex < activities.size()) return activities[currentIndex].get_name();
    return "All Done!";
}

bool Rate_24_Backend::isFinished() {
    return currentIndex >= activities.size();
}

// Generates the red warning texts based on user input
QString Rate_24_Backend::getWarningMessages(int realTime, int quality) {
    if (currentIndex >= activities.size()) return "";
    QString msgs = "";

    if (realTime < activities[currentIndex].get_min_time()) {
        msgs += "You did not work enough TIME\n";
    }
    // Αν ο χρόνος είναι 0, η ποιότητα δεν μετράει καθόλου, άρα δεν υπάρχει προειδοποίηση γι' αυτήν
    if (realTime > 0 && quality < activities[currentIndex].get_min_quality()) {
        msgs += "You did not work with enough QUALITY\n";
    }
    return msgs;
}

// Checks if the specific conditions for the SD/TO prompt are met
bool Rate_24_Backend::checkNeedsSD(int realTime, int quality) {
    if (currentIndex >= activities.size()) return false;
    if (realTime <= 0) return false; // χωρίς χρόνο, η ποιότητα αγνοείται → ούτε SD/TO ερώτηση
    int minTime = activities[currentIndex].get_min_time();
    int minQuality = activities[currentIndex].get_min_quality();
    return (realTime < minTime) && (quality > minQuality);
}

// Saves the data and advances to the next activity
void Rate_24_Backend::commitActivity(int realTime, int quality, bool isSD) {
    if (currentIndex < activities.size()) {
        // Άμυνα στα όρια (προστατεύει από υπερβολικές/αρνητικές τιμές)
        realTime = qBound(0, realTime, kMaxMinutes);
        quality  = qBound(0, quality, kMaxQuality);
        if (realTime == 0) quality = 0; // χωρίς χρόνο η ποιότητα δεν μετράει

        activities[currentIndex].set_real_time(realTime);
        activities[currentIndex].set_quality(quality);
        activities[currentIndex].set_is_sd(isSD); // Save the decision
        currentIndex++;
        saveDraft();
    }
}

// Undo της τελευταίας δραστηριότητας: γυρνάμε ένα βήμα πίσω και μηδενίζουμε τις τιμές της
bool Rate_24_Backend::undoLast() {
    if (currentIndex == 0) return false;
    currentIndex--;
    activities[currentIndex].set_real_time(0);
    activities[currentIndex].set_quality(0);
    activities[currentIndex].set_is_sd(false);
    saveDraft();
    return true;
}

int Rate_24_Backend::restoredCount() {
    return m_restoredCount;
}

// Calculates scores based strictly on your algorithm
// (κοινός υπολογισμός στο computeScores(): ίδια αποτελέσματα για κανονική εκτέλεση, vacation mode και επεξεργασία ημέρας)
// Η σημαντικότητα κάθε δραστηριότητας βασίζεται στον ΣΥΝΟΛΙΚΟ αριθμό δραστηριοτήτων, ώστε οι προτεραιότητες
// να έχουν το ίδιο βάρος και στο vacation mode. Το σκορ κανονικοποιείται ΜΟΝΟ στις δραστηριότητες που ισχύουν.
double Rate_24_Backend::calculateFinalScore() {
    std::vector<int> wi;
    const std::vector<ScoreIn> in = buildInputs(allActivities, activities, (int)activities.size(), wi);

    std::vector<double> worths, shares;
    const double score = computeScores(in, worths, shares);

    for (size_t i = 0; i < in.size(); ++i) {
        if (wi[i] >= 0) activities[wi[i]].set_total_worth(worths[i]);
    }
    return score;
}

// Formats the detailed scores list for the summary UI
QString Rate_24_Backend::getDetailedScores() {
    // ΠΑΡΕΝΕΡΓΕΙΑ: το calculateFinalScore() ενημερώνει και το total_worth κάθε δραστηριότητας,
    // γι' αυτό καλείται εδώ πριν διαβάσουμε τις τιμές.
    calculateFinalScore();
    QString details = "";
    for (const auto& act : activities) {
        details += act.get_name() + ": " + QString::number(act.get_total_worth(), 'f', 1) + " / 100\n";
    }
    return details;
}

// Pre-fills the notes area with activity names
QString Rate_24_Backend::getNotesTemplate() {
    QString templateStr = "";
    for (const auto& act : activities) {
        templateStr += act.get_name() + ":\n\n";
    }
    return templateStr;
}

// notes: το συνολικό κείμενο (όπως πριν) | activityNotes: μία σημείωση ανά δραστηριότητα, με τη σειρά των activities
void Rate_24_Backend::saveDay(const QString& notes, const QStringList& activityNotes) {
    QString filePath = historyFilePath();
    QJsonArray jsonArray;

    // 1. Read the existing JSON file if it exists (με προστασία corrupt JSON)
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close(); // Κλείνουμε άμεσα

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);

        if (err.error == QJsonParseError::NoError && doc.isArray()) {
            jsonArray = doc.array(); // Load history
        } else if (!data.isEmpty()) {
            // Backup σε περίπτωση σφάλματος ανάγνωσης
            QFile::remove(filePath + ".bak");
            QFile::copy(filePath, filePath + ".bak");
            qDebug() << "JSON parsing failed. Corrupt file backed up to" << filePath + ".bak";
        }
    }

    // 1b. Remove any previous entries of the same day (keep only the latest run)
    //     (εδώ φεύγουν και τα placeholders "missed"/"incomplete" της ίδιας ημέρας)
    const QString today = todayString();
    QJsonArray withoutToday;
    for (const QJsonValue& v : jsonArray) {
        if (v.toObject()["date"].toString() != today) {
            withoutToday.append(v);
        }
    }
    jsonArray = withoutToday;

    // 2. Create the new entry for today (όλες οι δραστηριότητες· όσες είναι εκτός vacation mode φέρουν "excluded")
    QJsonObject dayObject = makeDayObject(today, allActivities, activities, (int)activities.size(),
                                          notes, activityNotes, m_vacation);

    // Append today's data to the array
    jsonArray.append(dayObject);
    sortByDate(jsonArray);

    // 3. Write everything back to the file safely (Atomically)
    QSaveFile saveFile(filePath);
    if (saveFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(jsonArray);
        saveFile.write(doc.toJson()); // Save nicely formatted JSON

        if (saveFile.commit()) {
            qDebug() << "Data successfully and safely saved to" << filePath;
            clearDraft(); // η μέρα σώθηκε, το προσωρινό αρχείο δεν χρειάζεται πια
        } else {
            qDebug() << "Error: Commit failed, could not replace file.";
        }
    } else {
        qDebug() << "Error: Could not open save file for writing.";
    }
}

QVariantList Rate_24_Backend::getHistoryData(int daysLimit) {
    QVariantList history;
    QFile file(historyFilePath());

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            for (const QJsonValue& val : arr) {
                QJsonObject obj = val.toObject();
                QVariantMap map;
                map["date"] = obj["date"].toString();
                map["score"] = obj["score"].toDouble();
                map["vacation"] = obj["vacation"].toBool();
                history.append(map);
            }
        }
        file.close();
    }

    if (daysLimit > 0 && daysLimit < history.size()) {
        QVariantList filteredHistory;
        for (int i = history.size() - daysLimit; i < history.size(); ++i) {
            filteredHistory.append(history[i]);
        }
        return filteredHistory;
    }

    return history;
}

// Ιστορικό ανά δραστηριότητα για τις ίδιες τελευταίες N εγγραφές με το getHistoryData().
// Επιστρέφει: [ { name, rows: [ { date, score, skipped, vacation }, ... ] }, ... ]
// score = "share" (κλίμακα πλήρους ημέρας) ώστε οι μέρες vacation mode να συγκρίνονται δίκαια με τις κανονικές.
// Αν μια δραστηριότητα λείπει από κάποια μέρα, το σκορ της εκείνη τη μέρα είναι 0 (όπως στο CLI).
// skipped = η δραστηριότητα ήταν εκτός αξιολόγησης εκείνη τη μέρα (vacation mode).
QVariantList Rate_24_Backend::getActivityHistory(int daysLimit) {
    QJsonArray arr;
    QFile file(historyFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) arr = doc.array();
        file.close();
    }

    int start = 0;
    if (daysLimit > 0 && daysLimit < arr.size()) start = arr.size() - daysLimit;

    // Ονόματα δραστηριοτήτων με τη σειρά που εμφανίζονται
    QStringList names;
    for (int i = start; i < arr.size(); ++i) {
        const QJsonArray acts = arr[i].toObject()["activities"].toArray();
        for (const QJsonValue& a : acts) {
            const QString n = a.toObject()["name"].toString();
            if (!n.isEmpty() && !names.contains(n)) names.append(n);
        }
    }

    QVariantList result;
    for (const QString& n : names) {
        QVariantList rows;
        for (int i = start; i < arr.size(); ++i) {
            const QJsonObject day = arr[i].toObject();
            double value = 0.0;
            bool skipped = false;
            const QJsonArray acts = day["activities"].toArray();
            for (const QJsonValue& a : acts) {
                const QJsonObject o = a.toObject();
                if (o["name"].toString() == n) {
                    value = o.contains("share") ? o["share"].toDouble() : o["score"].toDouble();
                    skipped = o["excluded"].toBool();
                    break;
                }
            }
            QVariantMap row;
            row["date"] = day["date"].toString();
            row["score"] = value;
            row["skipped"] = skipped;
            row["vacation"] = day["vacation"].toBool();
            rows.append(row);
        }
        QVariantMap item;
        item["name"] = n;
        item["rows"] = rows;
        result.append(item);
    }
    return result;
}

// ---------- Draft (προσωρινή αποθήκευση ημιτελούς μέρας) ----------

void Rate_24_Backend::saveDraft() {
    QJsonObject obj;
    obj["date"] = todayString();
    obj["current_index"] = (int)currentIndex;
    obj["vacation"] = m_vacation;

    QJsonArray arr;
    for (const auto& act : activities) {
        QJsonObject o;
        o["name"] = act.get_name();
        o["real_time"] = act.get_real_time();
        o["quality"] = act.get_quality();
        o["sd_used"] = act.get_is_sd();
        arr.append(o);
    }
    obj["activities"] = arr;

    QFile f(draftFilePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        f.close();
    } else {
        qDebug() << "Warning: could not write draft file.";
    }
}

void Rate_24_Backend::clearDraft() {
    QFile::remove(draftFilePath());
}

void Rate_24_Backend::loadDraft() {
    QFile f(draftFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    if (obj["date"].toString() != todayString()) { // draft άλλης μέρας → άχρηστο
        clearDraft();
        return;
    }

    const QJsonArray arr = obj["activities"].toArray();
    const int idx = obj["current_index"].toInt(-1);
    const bool vac = obj["vacation"].toBool();

    // Το draft πρέπει να ταιριάζει με τις γνωστές δραστηριότητες (στο vacation mode είναι υποσύνολο)
    decltype(activities) work;
    if (!buildWorkingSet(allActivities, arr, vac, work)) return;
    if (idx < 0 || idx > (int)work.size()) return;

    for (int i = 0; i < idx; ++i) {
        QJsonObject o = arr[i].toObject();
        work[i].set_real_time(o["real_time"].toInt());
        work[i].set_quality(o["quality"].toInt());
        work[i].set_is_sd(o["sd_used"].toBool());
    }
    activities = work;
    currentIndex = idx;
    m_restoredCount = idx;
    m_vacation = vac;
}

// Draft παλιότερης μέρας (π.χ. το πρόγραμμα έκλεισε απότομα και δεν ξανάνοιξε την ίδια μέρα):
// γίνεται κανονική εγγραφή εκείνης της ημερομηνίας. Ό,τι δεν είχε συμπληρωθεί μένει 0.
void Rate_24_Backend::finalizeStaleDraft() {
    QFile f(draftFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) { clearDraft(); return; }

    const QJsonObject obj = doc.object();
    const QString date = obj["date"].toString();
    if (date == todayString()) return; // όχι παλιό: το χειρίζεται η loadDraft()

    const QJsonArray arr = obj["activities"].toArray();
    const int idx = obj["current_index"].toInt(-1);
    const bool vac = obj["vacation"].toBool();

    decltype(activities) work;
    const bool valid = QDate::fromString(date, "yyyy-MM-dd").isValid()
                       && buildWorkingSet(allActivities, arr, vac, work)
                       && idx >= 0 && idx <= (int)work.size();
    if (!valid) { clearDraft(); return; }

    QJsonArray history;
    if (!loadHistory(history)) return; // χαλασμένο αρχείο: δεν γράφουμε τίποτα, κρατάμε το draft

    const int existing = findByDate(history, date);
    if (existing < 0 || isPlaceholder(history[existing].toObject())) {
        for (int i = 0; i < idx; ++i) {
            const QJsonObject o = arr[i].toObject();
            work[i].set_real_time(o["real_time"].toInt());
            work[i].set_quality(o["quality"].toInt());
            work[i].set_is_sd(o["sd_used"].toBool());
        }
        QJsonObject entry = makeDayObject(date, allActivities, work, idx, QString(), QStringList(), vac);
        if (idx == 0) entry["missed"] = true; else entry["incomplete"] = true;
        putEntry(history, entry);
        if (!saveHistory(history)) return;
        qDebug() << "Stale draft of" << date << "was saved as an entry.";
    }
    loadDefaultActivities(); // καθαρή αρχική κατάσταση για τη σημερινή μέρα
    clearDraft();
}

// Συντήρηση ιστορικού στην εκκίνηση:
//  1) Παλιές εγγραφές χωρίς "priority" παίρνουν την τρέχουσα προτεραιότητα (μία φορά, μόνιμα).
//  2) Μέρες χωρίς καμία εγγραφή (δεν άνοιξε ο υπολογιστής/το πρόγραμμα) → εγγραφές με μηδενικά.
//     Ξεκινά από την πρώτη ημερομηνία του ιστορικού και φτάνει ως και χθες (η σημερινή μέρα δεν αγγίζεται).
void Rate_24_Backend::fillMissedDays() {
    QJsonArray history;
    if (!loadHistory(history) || history.isEmpty()) return;

    bool changed = stampPriorities(history, allActivities);

    QSet<QString> have;
    QDate first;
    for (const QJsonValue& v : history) {
        const QString s = v.toObject()["date"].toString();
        const QDate d = QDate::fromString(s, "yyyy-MM-dd");
        if (!d.isValid()) continue;
        have.insert(s);
        if (!first.isValid() || d < first) first = d;
    }

    if (first.isValid()) {
        const QDate today = effectiveNow().date();
        QDate from = first;
        if (from < today.addDays(-kMaxFillDays)) from = today.addDays(-kMaxFillDays);

        for (QDate d = from; d < today; d = d.addDays(1)) {
            const QString s = d.toString("yyyy-MM-dd");
            if (have.contains(s)) continue;
            QJsonObject entry = makeDayObject(s, allActivities, allActivities, 0, QString(), QStringList(), false);
            entry["missed"] = true;
            history.append(entry);
            changed = true;
        }
    }

    if (changed && saveHistory(history)) {
        qDebug() << "History maintenance done (priorities stamped / missed days filled).";
    }
}

// Καλείται όταν κλείνει το πρόγραμμα (X ή έξοδος). Αν η σημερινή ημέρα δεν έχει σωθεί κανονικά,
// καταγράφεται τώρα: ό,τι δεν συμπληρώθηκε = 0. Το draft μένει, ώστε αν ξανανοίξεις την ίδια μέρα
// να συνεχίσεις από εκεί που έμεινες (η κανονική αποθήκευση αντικαθιστά αυτή την εγγραφή).
void Rate_24_Backend::finalizeOnExit() {
    const QString today = todayString();
    QJsonArray history;
    if (!loadHistory(history)) return;

    const int existing = findByDate(history, today);
    if (existing >= 0 && !isPlaceholder(history[existing].toObject())) return; // ήδη σωστά σωσμένη

    const int done = qBound(0, (int)currentIndex, (int)activities.size());
    QJsonObject entry = makeDayObject(today, allActivities, activities, done, QString(), QStringList(), m_vacation);
    if (done == 0) entry["missed"] = true; else entry["incomplete"] = true;
    putEntry(history, entry);
    if (saveHistory(history)) {
        qDebug() << "Day" << today << "was closed before saving: recorded with zeros for what was missing.";
    }
}

// Επεξεργασία μιας ημέρας του ιστορικού. edits: ανά δραστηριότητα { real_time, quality, sd_used, note }.
// Τα σκορ υπολογίζονται ξανά με τον ίδιο αλγόριθμο όπως στην κανονική εκτέλεση.
// Δραστηριότητες εκτός αξιολόγησης (vacation mode) μένουν όπως είναι.
bool Rate_24_Backend::updateDay(const QString& date, const QVariantList& edits, const QString& legacyNotes) {
    QJsonArray history;
    if (!loadHistory(history)) return false;

    const int idx = findByDate(history, date);
    if (idx < 0) return false;

    QJsonObject day = history[idx].toObject();
    QJsonArray acts = day["activities"].toArray();
    const int n = acts.size();
    if (edits.size() != n) return false;

    bool hasNotes = false;
    for (const QJsonValue& v : acts) {
        if (v.toObject().contains("note")) hasNotes = true;
    }

    std::vector<ScoreIn> in;
    in.reserve(n);
    for (int i = 0; i < n; ++i) {
        QJsonObject o = acts[i].toObject();
        const QVariantMap e = edits[i].toMap();

        const int minT = o["min_time"].toInt();
        const int minQ = o["min_quality"].toInt();

        // Προτεραιότητα: ΠΑΝΤΑ από την ίδια την εγγραφή (όπως ήταν όταν καταγράφηκε η μέρα).
        // Μόνο αν λείπει (πολύ παλιά εγγραφή) παίρνεται από τον τρέχοντα ορισμό και αποθηκεύεται από εδώ και πέρα.
        int priority;
        if (o.contains("priority")) {
            priority = o["priority"].toInt();
        } else {
            priority = i + 1;
            const QString name = o["name"].toString();
            for (const auto& a : allActivities) {
                if (a.get_name() == name) { priority = a.get_priority(); break; }
            }
        }
        o["priority"] = priority;

        if (o["excluded"].toBool()) { // εκτός αξιολόγησης: δεν αλλάζει
            in.push_back({ 0, minT, 0, minQ, double(n - priority), false, false });
            acts[i] = o;
            continue;
        }

        int rt = qBound(0, e["real_time"].toInt(), kMaxMinutes);
        int q = qBound(0, e["quality"].toInt(), kMaxQuality);
        if (rt == 0) q = 0; // χωρίς χρόνο η ποιότητα δεν μετράει

        // Το SD επιτρέπεται μόνο στις συνθήκες της κανονικής εκτέλεσης (λίγος χρόνος, καλή ποιότητα)
        bool sd = e["sd_used"].toBool();
        if (!(rt > 0 && rt < minT && q > minQ)) sd = false;

        in.push_back({ rt, minT, q, minQ, double(n - priority), sd, true });
        o["real_time"] = rt;
        o["quality"] = q;
        o["sd_used"] = sd;
        if (hasNotes) o["note"] = e["note"].toString();
        acts[i] = o;
    }

    std::vector<double> worths, shares;
    const double dayScore = computeScores(in, worths, shares);
    for (int i = 0; i < n; ++i) {
        QJsonObject o = acts[i].toObject();
        o["score"] = worths[i];
        o["share"] = shares[i];
        acts[i] = o;
    }

    day["activities"] = acts;
    day["score"] = dayScore;

    if (hasNotes) {
        // Ξαναχτίζουμε το συνολικό κείμενο στη μορφή "Τίτλος:\nσημείωση\n\n" (ίδια με την κανονική αποθήκευση)
        QString combined;
        for (int i = 0; i < n; ++i) {
            const QJsonObject o = acts[i].toObject();
            if (o["excluded"].toBool()) continue;
            QString note = o["note"].toString();
            while (note.endsWith('\n') || note.endsWith(' ') || note.endsWith('\t')) note.chop(1);
            combined += o["name"].toString() + ":\n";
            combined += note.isEmpty() ? QString("\n") : note + "\n\n";
        }
        day["notes"] = combined;
    } else {
        day["notes"] = legacyNotes; // παλιές εγγραφές: μόνο το συνολικό κείμενο
    }

    day.remove("missed");
    day.remove("incomplete");
    day["edited"] = true;

    putEntry(history, day);
    return saveHistory(history);
}

// Επιστρέφει τα πλήρη στοιχεία μιας ημέρας:
// { found, date, score, notes, has_activity_notes, vacation, missed, incomplete, edited,
//   activities: [ { name, real_time, min_time, quality, min_quality, score, sd_used, note, excluded }, ... ] }
QVariantMap Rate_24_Backend::getDayDetails(const QString& date) {
    QVariantMap result;
    result["date"] = date;
    result["found"] = false;

    QJsonArray arr;
    QFile file(historyFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) arr = doc.array();
        file.close();
    }

    // Παίρνουμε την ΤΕΛΕΥΤΑΙΑ εγγραφή με αυτή την ημερομηνία (για παλιά αρχεία με διπλότυπα)
    QJsonObject found;
    bool ok = false;
    for (int i = arr.size() - 1; i >= 0; --i) {
        QJsonObject obj = arr[i].toObject();
        if (obj["date"].toString() == date) {
            found = obj;
            ok = true;
            break;
        }
    }
    if (!ok) return result;

    result["found"] = true;
    result["score"] = found["score"].toDouble();
    result["notes"] = found["notes"].toString();
    result["vacation"] = found["vacation"].toBool();
    result["missed"] = found["missed"].toBool();
    result["incomplete"] = found["incomplete"].toBool();
    result["edited"] = found["edited"].toBool();

    QVariantList list;
    bool hasActivityNotes = false;
    const QJsonArray acts = found["activities"].toArray();
    for (const QJsonValue& v : acts) {
        const QJsonObject o = v.toObject();
        QVariantMap m;
        m["name"] = o["name"].toString();
        m["real_time"] = o["real_time"].toInt();
        m["min_time"] = o["min_time"].toInt();
        m["quality"] = o["quality"].toInt();
        m["min_quality"] = o["min_quality"].toInt();
        m["score"] = o["score"].toDouble();
        m["sd_used"] = o["sd_used"].toBool();
        m["excluded"] = o["excluded"].toBool();
        if (o.contains("note")) {   // εγγραφές που σώθηκαν με σημειώσεις ανά δραστηριότητα
            hasActivityNotes = true;
            m["note"] = o["note"].toString();
        } else {
            m["note"] = QString();
        }
        list.append(m);
    }
    result["activities"] = list;
    result["has_activity_notes"] = hasActivityNotes;
    return result;
}

// Για την αυτόματη εκκίνηση από το systemd (ExecStart=... --auto):
// αν η σημερινή μέρα έχει ήδη σωθεί κανονικά, το πρόγραμμα δεν χρειάζεται να ανοίξει.
// Η χειροκίνητη εκτέλεση (χωρίς --auto) δεν επηρεάζεται ποτέ.
bool Rate_24_Backend::shouldSkipAutoLaunch() {
    if (!QCoreApplication::arguments().contains("--auto")) return false;

    QJsonArray history;
    if (!loadHistory(history)) return false;

    const int idx = findByDate(history, todayString());
    return idx >= 0 && !isPlaceholder(history[idx].toObject());
}

// Εξαγωγή ΟΛΩΝ των δεδομένων και των σημειώσεων σε ένα όμορφα στοιχισμένο αρχείο txt
// (~/Documents/Rate24/rate24_export.txt). Επιστρέφει τη διαδρομή του αρχείου, ή κενό αν απέτυχε.
QString Rate_24_Backend::exportToText() {
    QJsonArray history;
    if (!loadHistory(history)) return QString();
    sortByDate(history);

    const int W = 78;
    const QString heavy(W, '=');
    const QString light(W, '-');
    QStringList L;

    // --- Συνολικά στατιστικά ---
    const int days = history.size();
    double sum = 0.0, best = 0.0, maxVal = 0.1;
    QString bestDate;
    for (const QJsonValue& v : history) {
        const QJsonObject o = v.toObject();
        const double sc = o["score"].toDouble();
        sum += sc;
        if (sc > maxVal) maxVal = sc;
        if (bestDate.isEmpty() || sc > best) { best = sc; bestDate = o["date"].toString(); }
    }

    L << heavy;
    L << " RATE24 - FULL EXPORT";
    L << QString(" Generated : %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    if (days > 0) {
        L << QString(" Period    : %1  ->  %2   (%3 days)")
                 .arg(history.first().toObject()["date"].toString())
                 .arg(history.last().toObject()["date"].toString())
                 .arg(days);
        L << QString(" Average   : %1     Best: %2 (%3)")
                 .arg(sum / days, 0, 'f', 1).arg(best, 0, 'f', 1).arg(bestDate);
    } else {
        L << " No data yet.";
    }
    L << heavy << QString();

    // --- Επισκόπηση ---
    if (days > 0) {
        L << " OVERVIEW" << light;
        for (const QJsonValue& v : history) {
            const QJsonObject o = v.toObject();
            const double sc = o["score"].toDouble();
            const int filled = qBound(0, int(20 * sc / maxVal), 20);
            L << QString(" %1  %2  [%3%4]%5")
                     .arg(o["date"].toString())
                     .arg(sc, 7, 'f', 1)
                     .arg(QString(filled, '#'))
                     .arg(QString(20 - filled, '.'))
                     .arg(dayTag(o));
        }
        L << QString();
    }

    // --- Αναλυτικά ανά ημέρα ---
    struct Stat { double score = 0; double time = 0; double quality = 0; int qDays = 0; int tDays = 0; int n = 0; };
    QStringList order;
    QHash<QString, Stat> stats;

    for (const QJsonValue& v : history) {
        const QJsonObject day = v.toObject();
        const QJsonArray acts = day["activities"].toArray();

        L << heavy;
        L << QString(" %1    SCORE %2%3")
                 .arg(day["date"].toString())
                 .arg(day["score"].toDouble(), 0, 'f', 1)
                 .arg(dayTag(day));
        L << heavy;

        int nameW = 8;
        for (const QJsonValue& a : acts) nameW = qMax(nameW, a.toObject()["name"].toString().size());
        nameW = qMin(nameW, 30);

        L << " " + padRight("Activity", nameW) + "  " + padLeft("Time", 9) + "  " + padLeft("Quality", 9)
             + "  " + padLeft("SD", 3) + "  " + padLeft("Score", 7);
        L << " " + QString(nameW, '-') + "  " + QString(9, '-') + "  " + QString(9, '-')
             + "  " + QString(3, '-') + "  " + QString(7, '-');

        bool hasActivityNotes = false;
        for (const QJsonValue& a : acts) {
            const QJsonObject o = a.toObject();
            const QString name = o["name"].toString();
            if (o.contains("note")) hasActivityNotes = true;

            if (!order.contains(name)) order << name;

            if (o["excluded"].toBool()) {
                L << " " + padRight(name, nameW) + "  " + "skipped (vacation mode)";
                continue;
            }

            const int rt = o["real_time"].toInt();
            const int q = o["quality"].toInt();
            const double sc = o["score"].toDouble();
            const double share = o.contains("share") ? o["share"].toDouble() : sc;

            const QString timeStr = QString("%1/%2").arg(rt).arg(o["min_time"].toInt());
            const QString qualStr = rt > 0 ? QString("%1/%2").arg(q).arg(o["min_quality"].toInt()) : QString("-");
            L << " " + padRight(name, nameW) + "  " + padLeft(timeStr, 9) + "  " + padLeft(qualStr, 9)
                 + "  " + padLeft(o["sd_used"].toBool() ? "yes" : "-", 3) + "  " + padLeft(QString::number(sc, 'f', 1), 7);

            Stat& st = stats[name];
            st.n++;
            st.score += share;
            st.time += rt;
            if (rt > 0) { st.tDays++; st.quality += q; st.qDays++; }
        }
        L << QString();

        // Σημειώσεις
        if (hasActivityNotes) {
            bool any = false;
            for (const QJsonValue& a : acts) {
                const QJsonObject o = a.toObject();
                const QString note = o["note"].toString();
                if (note.trimmed().isEmpty()) continue;
                if (!any) { L << " NOTES"; any = true; }
                L << "   " + o["name"].toString();
                for (const QString& ln : wrapText(note, W - 8)) L << "     " + ln;
                L << QString();
            }
        } else if (!day["notes"].toString().trimmed().isEmpty()) {
            L << " NOTES";
            for (const QString& ln : wrapText(day["notes"].toString(), W - 6)) L << "   " + ln;
            L << QString();
        }
    }

    // --- Μέσοι όροι ανά δραστηριότητα ---
    if (!order.isEmpty()) {
        int nameW = 8;
        for (const QString& n : order) nameW = qMax(nameW, n.size());
        nameW = qMin(nameW, 30);

        L << heavy;
        L << " ACTIVITY AVERAGES (days the activity was evaluated; score on full-day scale)";
        L << heavy;
        L << " " + padRight("Activity", nameW) + "  " + padLeft("Avg score", 9) + "  " + padLeft("Avg time", 9)
             + "  " + padLeft("Avg qual.", 9) + "  " + padLeft("Days done", 9);
        L << " " + QString(nameW, '-') + "  " + QString(9, '-') + "  " + QString(9, '-')
             + "  " + QString(9, '-') + "  " + QString(9, '-');
        for (const QString& n : order) {
            const Stat st = stats.value(n);
            L << " " + padRight(n, nameW)
                 + "  " + padLeft(QString::number(st.n ? st.score / st.n : 0.0, 'f', 1), 9)
                 + "  " + padLeft(QString::number(st.n ? st.time / st.n : 0.0, 'f', 1), 9)
                 + "  " + padLeft(st.qDays ? QString::number(st.quality / st.qDays, 'f', 1) : QString("-"), 9)
                 + "  " + padLeft(QString("%1/%2").arg(st.tDays).arg(st.n), 9);
        }
        L << QString();
    }

    L << heavy;
    L << " END OF EXPORT";
    L << heavy;

    const QString path = QDir(dataDir()).filePath("rate24_export.txt");
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) return QString();
    out.write(L.join("\n").toUtf8());
    out.write("\n");
    if (!out.commit()) return QString();
    qDebug() << "Exported to" << path;
    return path;
}

// Προσωπικά ρεκόρ: συγκρίνει τη ΣΗΜΕΡΙΝΗ μέρα (όπως είναι τώρα στη μνήμη) με όλες τις άλλες μέρες του ιστορικού.
// Επιστρέφει: { hasHistory, vacation, score, prevDay, dayBest,
//               activities: [ { name, score, prev, best }, ... ] }
// - Ρεκόρ υπάρχει μόνο αν η νέα τιμή είναι αυστηρά μεγαλύτερη από μια προηγούμενη τιμή > 0.
// - Οι μέρες vacation mode ΔΕΝ μετράνε για ρεκόρ ημέρας (είναι "ευκολότερες"). Δεν εμφανίζεται ρεκόρ ημέρας αν η
//   σημερινή μέρα είναι vacation.
// - Τα ρεκόρ δραστηριοτήτων χρησιμοποιούν την κλίμακα πλήρους ημέρας ("share"), άρα συγκρίνονται δίκαια σε κάθε mode.
QVariantMap Rate_24_Backend::getBestInfo() {
    QVariantMap result;
    result["hasHistory"] = false;
    result["dayBest"] = false;
    result["prevDay"] = 0.0;
    result["vacation"] = m_vacation;

    const double score = calculateFinalScore(); // ενημερώνει και το total_worth κάθε δραστηριότητας
    result["score"] = score;

    std::vector<int> wi;
    const std::vector<ScoreIn> in = buildInputs(allActivities, activities, (int)activities.size(), wi);
    std::vector<double> worths, shares;
    computeScores(in, worths, shares);

    QJsonArray history;
    const bool ok = loadHistory(history);

    const QString today = todayString();
    double prevDay = 0.0;
    bool any = false;
    QHash<QString, double> prevAct;

    if (ok) {
        for (const QJsonValue& v : history) {
            const QJsonObject day = v.toObject();
            if (day["date"].toString() == today) continue; // η σημερινή ημερομηνία δεν συγκρίνεται με τον εαυτό της
            any = true;
            if (!day["vacation"].toBool())
                prevDay = qMax(prevDay, day["score"].toDouble());
            const QJsonArray acts = day["activities"].toArray();
            for (const QJsonValue& a : acts) {
                const QJsonObject o = a.toObject();
                if (o["excluded"].toBool()) continue;
                const QString name = o["name"].toString();
                const double val = o.contains("share") ? o["share"].toDouble() : o["score"].toDouble();
                prevAct[name] = qMax(prevAct.value(name, 0.0), val);
            }
        }
    }

    result["hasHistory"] = any;
    result["prevDay"] = prevDay;
    result["dayBest"] = !m_vacation && any && prevDay > 0.0 && score > prevDay;

    QVariantList list;
    for (size_t i = 0; i < in.size(); ++i) {
        if (wi[i] < 0) continue; // εκτός αξιολόγησης σήμερα
        const QString name = allActivities[i].get_name();
        const double cur = shares[i];
        const double prev = prevAct.value(name, 0.0);
        QVariantMap m;
        m["name"] = name;
        m["score"] = cur;
        m["prev"] = prev;
        m["best"] = any && prev > 0.0 && cur > prev;
        list.append(m);
    }
    result["activities"] = list;
    return result;
}
