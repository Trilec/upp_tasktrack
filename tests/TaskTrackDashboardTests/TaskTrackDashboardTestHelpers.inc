struct TestState {
    int passed = 0;
    Vector<String> failed;
    void Check(bool condition, const String& message) {
        if(condition) ++passed;
        else failed.Add(message);
    }
};

ValueMap Entry(const String& id, const String& title, const String& status, double progress,
               double weight = 1.0, bool attention = false)
{
    ValueMap e;
    e.Add("id", id);
    e.Add("title", title);
    e.Add("status", status);
    if(progress >= 0) e.Add("progress", progress);
    if(weight != 1.0) e.Add("weight", weight);
    if(attention) e.Add("attention", true);
    return e;
}

ValueMap Panel(const String& id, const String& type, const String& title,
               const ValueArray& entries, bool contributes = false)
{
    ValueMap p;
    p.Add("id", id);
    p.Add("type", type);
    p.Add("title", title);
    p.Add("category", "Overview");
    p.Add("density", "standard");
    p.Add("preview_limit", 6);
    p.Add("contributes_to_progress", contributes);
    p.Add("entries", entries);
    return p;
}
