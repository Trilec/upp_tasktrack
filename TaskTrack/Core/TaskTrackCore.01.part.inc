#include "TaskTrackCore.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

namespace Upp {

namespace {

bool IsSafeTaskId(const String& id)
{
    if(id.GetCount() < 8 || id.GetCount() > 120 || !id.StartsWith("task-"))
        return false;
    for(int i = 0; i < id.GetCount(); ++i) {
        int c = (byte)id[i];
        if(!(IsAlNum(c) || c == '-' || c == '_'))
            return false;
    }
    return true;
}

String ReadString(const Value& map, const char *key, const String& def = String())
{
    Value v = map[key];
    return IsNull(v) ? def : AsString(v);
}

bool ReadBool(const Value& map, const char *key, bool def)
{
    Value v = map[key];
    if(IsNull(v))
        return def;
    return v.Is<bool>() ? (bool)v : def;
}

int ReadInt(const Value& map, const char *key, int def)
{
    Value v = map[key];
    if(IsNull(v) || !IsNumber(v))
        return def;
    return (int)v;
}

int64 ReadInt64(const Value& map, const char *key, int64 def)
{
    Value v = map[key];
    if(IsNull(v) || !IsNumber(v))
        return def;
    return (int64)v;
}

double ReadDouble(const Value& map, const char *key, double def)
{
    Value v = map[key];
    if(IsNull(v) || !IsNumber(v))
        return def;
    return (double)v;
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

bool ValidateColorText(const String& color)
{
    if(color.IsEmpty())
        return true;
    String s = color;
    if(s.StartsWith("#"))
        s = s.Mid(1);
    if(s.GetCount() != 6 && s.GetCount() != 8)
        return false;
    for(int i = 0; i < s.GetCount(); ++i)
        if(!isxdigit((unsigned char)s[i]))
            return false;
    return true;
}

String EnsureStoreRoot(const String& requested)
{
    String root = TrimBoth(requested);
    if(root.IsEmpty())
        root = TaskTrackDefaultStoreRoot();
    return NormalizePath(root);
}

bool AtomicSaveText(const String& path, const String& text, String& error)
{
    String folder = GetFileFolder(path);
    if(!RealizeDirectory(folder)) {
        error = "Unable to create TaskTrack storage folder: " + folder;
        return false;
    }

    String temp = path + ".tmp";
    String backup = path + ".bak";

    if(!SaveFile(temp, text)) {
        error = "Unable to write temporary TaskTrack file: " + temp;
        return false;
    }

    String verify = LoadFile(temp);
    if(IsNull(verify) || verify != text) {
        FileDelete(temp);
        error = "TaskTrack temporary-file verification failed: " + temp;
        return false;
    }

    if(FileExists(path)) {
        FileDelete(backup);
        if(!FileCopy(path, backup)) {
            FileDelete(temp);
            error = "Unable to create TaskTrack recovery backup: " + backup;
            return false;
        }
    }

    if(FileExists(path) && !FileDelete(path)) {
        FileDelete(temp);
        error = "Unable to replace TaskTrack file: " + path;
        return false;
    }

    if(!FileMove(temp, path)) {
        if(FileExists(backup))
            FileCopy(backup, path);
        FileDelete(temp);
        error = "Unable to install TaskTrack file: " + path;
        return false;
    }

    return true;
}

bool ParseTaskText(const String& text, TaskTrackDocument& doc, String& error)
{
    if(IsNull(text)) {
        error = "TaskTrack task file is unreadable.";
        return false;
    }
    Value parsed;
    try {
        parsed = ParseJSON(text);
    }
    catch(CParser::Error) {
        error = "Invalid TaskTrack JSON.";
        return false;
    }
    return TaskTrackFromValue(parsed, doc, error);
}

String MarkdownEscape(String text)
{
    text.Replace("\r", "");
    text.Replace("\n", " ");
    return text;
}

String DataToDisplay(const Value& data)
{
    if(IsNull(data))
        return String();
    if(data.Is<ValueArray>() || data.Is<ValueMap>())
        return AsJSON(data, false);
    return AsString(data);
}

Value AnswerToValue(const TaskTrackAnswer& answer)
{
    ValueMap m;
    m.Add("answered", answer.answered);
    m.Add("status", answer.status);
    m.Add("value", answer.value);
    m.Add("data", answer.data);
    m.Add("note", answer.note);
    m.Add("answered_at", answer.answered_at);
    return Value(m);
}

bool ParseAnswer(const Value& raw, TaskTrackAnswer& answer, const String& prefix, String& error)
{
    if(IsNull(raw))
        return true;
    if(!raw.Is<ValueMap>()) {
