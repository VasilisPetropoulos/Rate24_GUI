#include "Activity.h"

Activity::Activity() {
    name = "";
    min_time = 0;
    real_time = 0;
    min_quality = 0;
    quality = 0;
    priority = 0;
    total_worth = 0.0;
    is_sd = false;
}

Activity::Activity(const QString& name, int min_time, int min_quality, int priority) {
    this->name = name;
    this->min_time = min_time;
    this->real_time = 0;
    this->min_quality = min_quality;
    this->quality = 0;
    this->priority = priority;
    this->total_worth = 0.0;
    this->is_sd = false;
}

// Setters
void Activity::set_real_time(int real_time) { this->real_time = real_time; }
void Activity::set_quality(int quality) { this->quality = quality; }
void Activity::set_total_worth(float total_worth) { this->total_worth = total_worth; }
void Activity::set_is_sd(bool is_sd) { this->is_sd = is_sd; }

// Getters
QString Activity::get_name() const { return name; }
int Activity::get_min_time() const { return min_time; }
int Activity::get_real_time() const { return real_time; }
int Activity::get_min_quality() const { return min_quality; }
int Activity::get_quality() const { return quality; }
int Activity::get_priority() const { return priority; }
float Activity::get_total_worth() const { return total_worth; }
bool Activity::get_is_sd() const { return is_sd; }
