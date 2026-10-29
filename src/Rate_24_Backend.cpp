#include "Rate_24_Backend.h"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QVariantList>
#include <QDebug>
#include <QtGlobal>
#include <QStandardPaths>
#include <QDir>

namespace {
constexpr int kMaxQuality  = 100;
constexpr int kMaxMinutes  = 24 * 60;

QString todayString() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd");
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
} // namespace

Rate_24_Backend::Rate_24_Backend(QObject *parent) : QObject(parent) {
    loadDefaultActivities();
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

    currentIndex = 0;
}

int Rate_24_Backend::getActivityCount() { return activities.size(); }

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
double Rate_24_Backend::calculateFinalScore() {
    double day_score = 0.0;
    double max_possible_score = 0.0;
    int num_activities = activities.size();

    for (const auto& act : activities) {
        double act_importance = num_activities - act.get_priority();
        max_possible_score += 1.0 * act_importance;
    }

    if (max_possible_score == 0.0) return 0.0;

    for (auto& act : activities) {
        double act_importance = num_activities - act.get_priority();
        double justify_management = 0.0;

        // Προστασία από διαίρεση με το 0 (min_time / min_quality = 0 σημαίνει "δεν υπάρχει απαίτηση")
        // και αν ο χρόνος είναι 0, ούτε η ολοκλήρωση ούτε η ποιότητα μετράνε.
        double completion_ratio = 0.0;
        double quality_ratio = 0.0;

        if (act.get_real_time() > 0) {
            completion_ratio = (act.get_min_time() > 0)
                ? (double)act.get_real_time() / act.get_min_time()
                : 1.0;
            quality_ratio = (act.get_min_quality() > 0)
                ? (double)act.get_quality() / act.get_min_quality()
                : 1.0;
        }

        double total_ratio = completion_ratio * quality_ratio;

        // Apply SD bonus if selected
        if (act.get_is_sd()) {
            justify_management = 0.10 * max_possible_score;
        }

        act.set_total_worth((((total_ratio * act_importance) + justify_management) / max_possible_score) * 100.0);
        day_score += (total_ratio * act_importance) + justify_management;
    }

    return (day_score / max_possible_score) * 100.0;
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
    double score = calculateFinalScore();
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
    const QString today = todayString();
    QJsonArray withoutToday;
    for (const QJsonValue& v : jsonArray) {
        if (v.toObject()["date"].toString() != today) {
            withoutToday.append(v);
        }
    }
    jsonArray = withoutToday;

    // 2. Create the new entry for today
    QJsonObject dayObject;
    dayObject["date"] = today;
    dayObject["score"] = score;
    dayObject["notes"] = notes;

    // Save detailed breakdown per activity
    QJsonArray activitiesArray;

    for (int i = 0; i < (int)activities.size(); ++i) {
        const auto& act = activities[i];
        QJsonObject actObj;
        actObj["name"] = act.get_name();
        actObj["real_time"] = act.get_real_time();
        actObj["min_time"] = act.get_min_time();
        actObj["quality"] = act.get_quality();
        actObj["min_quality"] = act.get_min_quality();
        actObj["score"] = act.get_total_worth();
        actObj["sd_used"] = act.get_is_sd();
        actObj["note"] = (i < activityNotes.size()) ? activityNotes[i] : QString();
        activitiesArray.append(actObj);
    }
    dayObject["activities"] = activitiesArray;

    // Append today's data to the array
    jsonArray.append(dayObject);

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
// Επιστρέφει: [ { name: "...", rows: [ { date: "...", score: 12.3 }, ... ] }, ... ]
// Αν μια δραστηριότητα λείπει από κάποια μέρα, το σκορ της εκείνη τη μέρα είναι 0 (όπως στο CLI).
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
            const QJsonArray acts = day["activities"].toArray();
            for (const QJsonValue& a : acts) {
                const QJsonObject o = a.toObject();
                if (o["name"].toString() == n) {
                    value = o["score"].toDouble();
                    break;
                }
            }
            QVariantMap row;
            row["date"] = day["date"].toString();
            row["score"] = value;
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

    QJsonArray arr = obj["activities"].toArray();
    int idx = obj["current_index"].toInt(-1);
    int count = (int)activities.size();
    if (arr.size() != count || idx < 0 || idx > count) return;

    // Το draft πρέπει να ταιριάζει με τις τρέχουσες δραστηριότητες
    for (int i = 0; i < count; ++i) {
        if (arr[i].toObject()["name"].toString() != activities[i].get_name()) return;
    }

    for (int i = 0; i < idx; ++i) {
        QJsonObject o = arr[i].toObject();
        activities[i].set_real_time(o["real_time"].toInt());
        activities[i].set_quality(o["quality"].toInt());
        activities[i].set_is_sd(o["sd_used"].toBool());
    }
    currentIndex = idx;
    m_restoredCount = idx;
}

// Πρόσθεσε αυτή τη συνάρτηση στο ΤΕΛΟΣ του Rate_24_Backend.cpp (δεν αλλάζει τίποτα άλλο).
//
// Επιστρέφει τα πλήρη στοιχεία μιας ημέρας:
// { found, date, score, notes, has_activity_notes,
//   activities: [ { name, real_time, min_time, quality, min_quality, score, sd_used, note }, ... ] }
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
