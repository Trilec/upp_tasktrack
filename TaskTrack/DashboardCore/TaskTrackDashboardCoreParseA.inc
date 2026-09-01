namespace {

String ReadString(const Value& map, const char *key, const String& def = String())
{
    Value v = map[key];
    return IsNull(v) ? def : AsString(v);
}

bool ReadBool(const Value& map, const char *key, bool def)
{
    Value v = map[key];
    return IsNull(v) ? def : (bool)v;
}

int ReadInt(const Value& map, const char *key, int def)
{
    Value v = map[key];
    return IsNull(v) || !IsNumber(v) ? def : (int)v;
}

double ReadDouble(const Value& map, const char *key, double def)
{
    Value v = map[key];
    return IsNull(v) || !IsNumber(v) ? def : (double)v;
}

bool ExpectOptionalString(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || v.Is<String>())
        return true;
    error = String(key) + " must be a string.";
    return false;
}

bool ExpectOptionalBool(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || v.Is<bool>())
        return true;
    error = String(key) + " must be a boolean.";
    return false;
}

bool ExpectOptionalNumber(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || IsNumber(v))
        return true;
    error = String(key) + " must be numeric.";
    return false;
}

bool ExpectOptionalArray(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || v.Is<ValueArray>())
        return true;
    error = String(key) + " must be an array.";
    return false;
}

bool IsSafeDashboardId(const String& id)
{
    if(id.GetCount() < 11 || id.GetCount() > 120 || !id.StartsWith("dashboard-"))
        return false;
    for(int i = 0; i < id.GetCount(); ++i) {
        int c = (byte)id[i];
        if(!(IsAlNum(c) || c == '-' || c == '_'))
            return false;
    }
    return true;
}

String EnsureStoreRoot(const String& requested)
{
    String root = TrimBoth(requested);
    if(root.IsEmpty())
        root = TaskTrackDashboardDefaultStoreRoot();
    return NormalizePath(root);
}

String DashboardWriteLockPath(const String& store_root, const String& dashboard_id)
{
    return AppendFileName(EnsureStoreRoot(store_root),
                          dashboard_id + ".tasktrack-dashboard.lock");
}

class DashboardWriteLock {
public:
    bool Acquire(const String& store_root, const String& dashboard_id, String& error)
    {
        String root = EnsureStoreRoot(store_root);
        if(!RealizeDirectory(root)) {
            error = "Unable to create TaskTrack dashboard storage folder: " + root;
            return false;
        }
        path_ = DashboardWriteLockPath(root, dashboard_id);
        for(int attempt = 0; attempt < 40; ++attempt) {
            if(stream_.Open(~path_, FileStream::READWRITE | FileStream::NOWRITESHARE))
                return true;
            Sleep(25);
        }
        error = "TaskTrack dashboard is busy with another writer: " + dashboard_id;
        return false;
    }

private:
    String path_;
    FileStream stream_;
};

bool AtomicSaveText(const String& path, const String& text, String& error)
{
    String folder = GetFileFolder(path);
    if(!RealizeDirectory(folder)) {
        error = "Unable to create TaskTrack dashboard storage folder: " + folder;
        return false;
    }

    String temp = path + ".tmp";
    String backup = path + ".bak";
    if(!SaveFile(temp, text)) {
        error = "Unable to write temporary TaskTrack dashboard file: " + temp;
        return false;
    }
    String verify = LoadFile(temp);
    if(IsNull(verify) || verify != text) {
        FileDelete(temp);
        error = "TaskTrack dashboard temporary-file verification failed: " + temp;
        return false;
    }

    if(FileExists(path)) {
        FileDelete(backup);
        if(!FileCopy(path, backup)) {
            FileDelete(temp);
            error = "Unable to create TaskTrack dashboard recovery backup: " + backup;
            return false;
        }
    }

    if(FileExists(path) && !FileDelete(path)) {
        FileDelete(temp);
        error = "Unable to replace TaskTrack dashboard file: " + path;
        return false;
    }

    if(!FileMove(temp, path)) {
        if(FileExists(backup))
            FileCopy(backup, path);
        FileDelete(temp);
        error = "Unable to install TaskTrack dashboard file: " + path;
        return false;
    }
    return true;
}

bool ParseDashboardText(const String& text, TaskTrackDashboardDocument& doc, String& error)
{
    if(IsNull(text)) {
        error = "TaskTrack dashboard file is unreadable.";
        return false;
    }
    Value parsed;
    try {
        parsed = ParseJSON(text);
    }
    catch(CParser::Error) {
        error = "Invalid TaskTrack dashboard JSON.";
        return false;
    }
    return TaskTrackDashboardFromValue(parsed, doc, error);
}

bool StatusImpliesAttention(const String& raw)
{
    String s = ToLower(TrimBoth(raw));
    return s == "blocked" || s == "fail" || s == "failed" || s == "error" ||
           s == "warning" || s == "attention" || s == "risk" || s == "critical";
}
