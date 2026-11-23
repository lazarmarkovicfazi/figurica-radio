#include "EventLog.h"

#include <stdarg.h>

namespace {

constexpr int LOG_ENTRIES = 80;
constexpr int LOG_LINE = 80;

struct Entry {
  uint32_t ms;
  char text[LOG_LINE];
};

Entry entries[LOG_ENTRIES];
int nextEntry = 0;
int usedEntries = 0;

}  // namespace

void logf(const char* fmt, ...) {
  Entry& e = entries[nextEntry];
  e.ms = millis();
  va_list args;
  va_start(args, fmt);
  vsnprintf(e.text, sizeof(e.text), fmt, args);
  va_end(args);

  nextEntry = (nextEntry + 1) % LOG_ENTRIES;
  if (usedEntries < LOG_ENTRIES) usedEntries++;
  Serial.println(e.text);
}

void logDump() {
  Serial.printf("[log] poslednjih %d dogadjaja, vreme je u sekundama od paljenja:\n", usedEntries);
  const int first = (nextEntry - usedEntries + LOG_ENTRIES) % LOG_ENTRIES;
  for (int i = 0; i < usedEntries; i++) {
    const Entry& e = entries[(first + i) % LOG_ENTRIES];
    Serial.printf("%6lu.%lu  %s\n", (unsigned long)(e.ms / 1000), (unsigned long)((e.ms / 100) % 10),
                  e.text);
  }
}
