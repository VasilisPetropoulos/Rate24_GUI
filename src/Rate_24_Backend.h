#ifndef RATE_24_BACKEND_H
#define RATE_24_BACKEND_H

#include <QObject>
#include <QString>
#include <vector>
#include <QDebug>
#include <QVariantList>
#include <QVariantMap>
#include "Activity.h"

class Rate_24_Backend : public QObject {
    Q_OBJECT
public:
    explicit Rate_24_Backend(QObject *parent = nullptr);

    Q_INVOKABLE void loadDefaultActivities();
    Q_INVOKABLE int getActivityCount();

    Q_INVOKABLE QString getCurrentActivityName();
    Q_INVOKABLE bool isFinished();
    Q_INVOKABLE bool undoLast();
    Q_INVOKABLE int  restoredCount();

    // New validation and workflow methods
    Q_INVOKABLE QString getWarningMessages(int realTime, int quality);
    Q_INVOKABLE bool checkNeedsSD(int realTime, int quality);
    Q_INVOKABLE void commitActivity(int realTime, int quality, bool isSD);

    // End-of-day summary generation
    Q_INVOKABLE double calculateFinalScore();
    Q_INVOKABLE QString getDetailedScores();
    Q_INVOKABLE QString getNotesTemplate();
    Q_INVOKABLE void saveDay(const QString& notes, const QStringList& activityNotes);
    Q_INVOKABLE QVariantList getHistoryData(int daysLimit = 14);
    Q_INVOKABLE QVariantList getActivityHistory(int daysLimit);
    Q_INVOKABLE QVariantMap getDayDetails(const QString& date);

private:
    std::vector<Activity> activities;
    size_t currentIndex = 0;
    void saveDraft();
    void loadDraft();
    void clearDraft();
    int  m_restoredCount = 0;
};

#endif