/* Some stuff for 6.829 */

#include <linux/time.h>
#include "myglobs.h"
#include "ath9k.h"

struct mutex ath_myglobs_mutex;

char ath_stats_buffer[8192][64];
int ath_stats_buffer_idx;

struct ath_rate_table *ath_current_rate_table = 0;
struct ieee80211_tx_info *ath_current_tx_info = 0;

struct timespec ath_packet_send_start = { 0 };
struct timespec ath_packet_send_end = { 0 };
struct timespec ath_packet_send_id = { 0 };
unsigned long ath_packet_send_diff = 0;
u8 ath_packet_send_rate = 0;
u8 ath_packet_send_retries = 0;

static u8 ath_rotating_rix = 0;

void ath_myglobs_init() {
  ath_stats_buffer_idx = 0;
  mutex_init(&ath_myglobs_mutex);
}

void ath_myglobs_lock() {
  mutex_lock(&ath_myglobs_mutex);
}

void ath_myglobs_unlock() {
  mutex_unlock(&ath_myglobs_mutex);
}

struct ath_rate_table *ath_get_current_rate_table() {
  return ath_current_rate_table;
}

void ath_set_current_rate_table(struct ath_rate_table *table) {
  ath_current_rate_table = table;
}

struct ieee80211_tx_info *ath_get_current_tx_info() {
  return ath_current_tx_info;
}

void ath_set_current_tx_info(struct ieee80211_tx_info *tx_info) {
  ath_current_tx_info = tx_info;
}

void ath_set_on_send() {
  ath_myglobs_lock();
  getnstimeofday(&ath_packet_send_start);
  if (ath_current_tx_info == 0) {
      // do nothing, keep send rate at 0
  } else {
      ath_packet_send_rate = ath_current_tx_info->control.rates[0].idx;
  }
}

void ath_set_on_complete() {
  struct timespec diff;
  getnstimeofday(&ath_packet_send_end);
  diff = timespec_sub(ath_packet_send_end, ath_packet_send_start);
  ath_packet_send_id = ath_packet_send_start;
  ath_packet_send_diff = diff.tv_sec * 1000000000L + diff.tv_nsec;
  if (ath_current_tx_info == 0) {
    //do nothing, keep tries at 0
  } else {
    ath_packet_send_retries = ath_current_tx_info->control.rates[0].count;
  }
  ath_inc_rotating_rix();
  ath_myglobs_unlock();
}

unsigned long ath_get_send_id() {
  return ath_packet_send_id.tv_sec * 1000000000L + ath_packet_send_id.tv_nsec;
}

unsigned long ath_get_send_diff() {
  return ath_packet_send_diff;
}

u8 ath_get_send_tries() {
  return ath_packet_send_retries;
}

u8 ath_get_send_rate() {
  return ath_packet_send_rate;
}

void ath_inc_rotating_rix() {
  if (ath_rotating_rix < 12) {
    ath_rotating_rix++;
  } else {
    ath_rotating_rix = 0;
  }
}

u8 ath_get_rotating_rix() {
  return ath_rotating_rix;
}

int ath_stats_to_str(char *buffer, size_t buffer_length) {
  struct ath_rate_table *table = ath_get_current_rate_table();
  int i, ret;
  ret = 0;
  
  i  = ath_get_send_rate();

  ret += snprintf(buffer + ret, buffer_length - ret,
                  "Last(%lu) took %lu ns / %d tries with rate %d at %d(%d) kbps\n",
                  ath_get_send_id(),
                  ath_get_send_diff(),
                  ath_get_send_tries(),
                  i,
                  table->info[i].ratekbps,
                  table->info[i].user_ratekbps);
  if (ret >= buffer_length) {
    buffer[buffer_length] = '\0';
  }
  return ret;
}

void ath_set_buffer() {
  ath_myglobs_lock();
  if (ath_stats_buffer_idx < sizeof(ath_stats_buffer) / sizeof(ath_stats_buffer[0])) {
    ath_stats_to_str(ath_stats_buffer[ath_stats_buffer_idx], 64);
    ath_stats_buffer_idx++;
  }
  ath_myglobs_unlock();
}

int ath_get_buffer(char *buffer, size_t buffer_length) {
  int i, ret;
  ret = 0;
  
  ath_myglobs_lock();
  for (i = 0; i < ath_stats_buffer_idx; i++) {
    ret += snprintf(buffer + ret, buffer_length - ret,
                    "%s", ath_stats_buffer[i]);
  }
  ath_stats_buffer_idx = 0;
  ath_myglobs_unlock();

  return ret;
}
