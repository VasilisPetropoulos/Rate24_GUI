#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <QString>

class Activity {
    public:
        Activity();
        Activity(const QString& name, int min_time, int min_quality, int priority);

        void set_real_time(int real_time);
        void set_quality(int quality);
        void set_total_worth(float total_worth);
        void set_is_sd(bool sd); // Setter for SD

        QString get_name() const;
        int get_min_time() const;
        int get_real_time() const;
        int get_min_quality() const;
        int get_quality() const;
        int get_priority() const;
        float get_total_worth() const;
        bool get_is_sd() const;

    private:
        QString name;
        int min_time;
        int real_time;
        int min_quality;
        int quality;
        int priority;
        float total_worth;  // out of 100
        bool is_sd;
};

#endif
