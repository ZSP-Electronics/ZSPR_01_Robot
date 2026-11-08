#include "NVS_Store.h"

#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>

namespace
{
    // One lock for all NVS access (Preferences itself is not thread-safe).
    SemaphoreHandle_t nvsLock()
    {
        static SemaphoreHandle_t m = xSemaphoreCreateMutex(); // thread-safe static init
        return m;
    }

    // Opens a namespace for the life of the object, holding the lock.
    class Session
    {
    public:
        Session(const char *ns, bool readOnly)
        {
            xSemaphoreTake(nvsLock(), portMAX_DELAY);
            _open = prefs.begin(ns, readOnly);
        }
        ~Session()
        {
            if (_open)
                prefs.end();
            xSemaphoreGive(nvsLock());
        }
        Session(const Session &) = delete;
        Session &operator=(const Session &) = delete;

        bool ok() const { return _open; }
        Preferences prefs;

    private:
        bool _open = false;
    };
} // namespace

namespace NVS
{
    bool saveBytes(const char *key, const void *data, size_t len, const char *ns)
    {
        if (!key || !data || len == 0)
            return false;
        Session s(ns, false);
        return s.ok() && s.prefs.putBytes(key, data, len) == len;
    }

    bool loadBytes(const char *key, void *data, size_t len, const char *ns)
    {
        if (!key || !data || len == 0)
            return false;
        Session s(ns, true);
        if (!s.ok() || s.prefs.getBytesLength(key) != len)
            return false;
        return s.prefs.getBytes(key, data, len) == len;
    }

    bool saveString(const char *key, const char *text, const char *ns)
    {
        if (!key || !text)
            return false;
        Session s(ns, false);
        return s.ok() && s.prefs.putString(key, text) == strlen(text);
    }

    bool loadString(const char *key, String &out, const char *ns)
    {
        if (!key)
            return false;
        Session s(ns, true);
        if (!s.ok() || !s.prefs.isKey(key))
            return false;
        out = s.prefs.getString(key);
        return true;
    }

    bool exists(const char *key, const char *ns)
    {
        if (!key)
            return false;
        Session s(ns, true);
        return s.ok() && s.prefs.isKey(key);
    }

    bool erase(const char *key, const char *ns)
    {
        if (!key)
            return false;
        Session s(ns, false);
        return s.ok() && s.prefs.remove(key);
    }

    bool eraseAll(const char *ns)
    {
        Session s(ns, false);
        return s.ok() && s.prefs.clear();
    }
} // namespace NVS
