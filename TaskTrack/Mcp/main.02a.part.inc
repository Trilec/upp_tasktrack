{
    ValueMap props;
    props.Add("task_id", StringSchema("Stable TaskTrack task id returned by create_task."));
    props.Add("store_root", StringSchema("Optional store used when the task was created outside the default store."));
    if(include_items) props.Add("include_items", BoolSchema("Include question definitions and structured answers."));
    ValueArray required; required.Add("task_id");
    ValueMap s;
    s.Add("type", "object"); s.Add("required", required); s.Add("properties", props); s.Add("additionalProperties", false);
    return Value(s);
}

Value GradientSchema()
{
    ValueMap props;
    props.Add("id", StringSchema("Stable option value returned to the agent."));
    props.Add("label", StringSchema("Short human label."));
    props.Add("from", StringSchema("Start colour as #RRGGBB."));
    props.Add("to", StringSchema("End colour as #RRGGBB."));
    ValueArray required; required.Add("id"); required.Add("from"); required.Add("to");
    ValueMap item; item.Add("type", "object"); item.Add("required", required); item.Add("properties", props); item.Add("additionalProperties", false);
    ValueMap array; array.Add("type", "array"); array.Add("items", item);
    return Value(array);
}

Value HierarchySchema()
{
    ValueMap props;
    props.Add("id", StringSchema("Stable node value returned to the agent."));
    props.Add("parent_id", StringSchema("Parent id; empty for a root-level node."));
    props.Add("label", StringSchema("Human-facing node label."));
    ValueArray required; required.Add("id"); required.Add("label");
    ValueMap item; item.Add("type", "object"); item.Add("required", required); item.Add("properties", props); item.Add("additionalProperties", false);
    ValueMap array; array.Add("type", "array"); array.Add("items", item);
    return Value(array);
}

Value CreateTaskSchema()
{
    ValueArray type_enum;
    const char *types[] = {
        "confirm", "single_choice", "multi_choice", "select", "list_select",
        "text", "notes", "number", "amount", "range", "rating", "color",
        "gradient", "position", "direction", "rank_order", "hierarchy_select", "curve"
    };
    for(const char *type : types) type_enum.Add(type);

    ValueMap type_schema;
    type_schema.Add("type", "string");
    type_schema.Add("enum", type_enum);
    type_schema.Add("description",
        "Semantic human response type. Choose by meaning, never by GUI widget: confirm=yes/no; single_choice=one of choices; multi_choice=several choices; select=compact populated selection; list_select=larger visible list; text=short exact text; notes=qualification/free text; number=exact numeric value; amount=magnitude on a scale; range=low/high bounds; rating=small ordinal score; color=colour choice; gradient=visual gradient choice; position=3x3 placement; direction=8-way direction; rank_order=dragged priority order; hierarchy_select=tree node; curve=easing/falloff curve." );

    ValueMap item_props;
    item_props.Add("id", StringSchema("Stable item id used in results and missing-required reports."));
    item_props.Add("category", StringSchema("Short grouping label. Use categories only when they help scanning."));
    item_props.Add("type", type_schema);
    item_props.Add("title", StringSchema("Concise human question, normally one line."));
    item_props.Add("instruction", StringSchema("Short concrete instruction or decision criterion. Avoid repeating the title."));
    item_props.Add("required", BoolSchema("True only when the agent cannot continue without an answer."));
    item_props.Add("choices", StringArraySchema("Discrete options for single_choice, multi_choice, select, list_select, rank_order; optionally two labels for confirm."));
    item_props.Add("allow_multiple", BoolSchema("Allow multiple selections for list_select or hierarchy_select."));
    item_props.Add("recommended", StringSchema("Optional agent recommendation shown separately; it never pre-answers the question."));
    item_props.Add("min", NumberSchema("Lower bound for number/amount/range/rating."));
    item_props.Add("max", NumberSchema("Upper bound for number/amount/range/rating."));
    item_props.Add("step", NumberSchema("Positive numeric increment."));
    item_props.Add("unit", StringSchema("Short suffix such as px, %, ms, or variants."));
    item_props.Add("colors", StringArraySchema("Preset #RRGGBB colours for color."));
    item_props.Add("gradients", GradientSchema());
    item_props.Add("hierarchy", HierarchySchema());
    item_props.Add("default", ValueMap());

    ValueArray item_required; item_required.Add("type"); item_required.Add("title");
    ValueMap item_schema;
    item_schema.Add("type", "object"); item_schema.Add("required", item_required); item_schema.Add("properties", item_props); item_schema.Add("additionalProperties", false);

    ValueMap items;
    items.Add("type", "array"); items.Add("minItems", 1); items.Add("items", item_schema);
    items.Add("description", "Questions to present. Prefer structured semantic types over asking for prose.");

    ValueMap props;
    props.Add("task_id", StringSchema("Optional caller-supplied id beginning task-. Normally omit."));
    props.Add("project", StringSchema("Short project/product name."));
    props.Add("title", StringSchema("Overall objective the human is helping to resolve."));
    props.Add("subtitle", StringSchema("Optional one-line context."));
    props.Add("actor", StringSchema("Calling agent/session label."));
    props.Add("store_root", StringSchema("Optional durable store directory."));
    props.Add("reminder_minutes", IntSchema(0, 1440, "Inactivity reminder interval; 0 disables reminders."));
    props.Add("remind_while_paused", BoolSchema("Whether inactivity reminders may also appear for explicitly paused tasks."));
    props.Add("nudge_on_agent_poll", BoolSchema("Let a later agent poll gently remind an inactive human without changing task state."));
    props.Add("history_limit", IntSchema(5, 200, "Bound retained completed/closed task history. Active work is never pruned for this limit."));
    props.Add("launch", BoolSchema("Launch the TaskTrack GUI after durable creation. Defaults true."));
    props.Add("items", items);

    ValueArray required; required.Add("title"); required.Add("items");
    ValueMap s;
    s.Add("type", "object"); s.Add("required", required); s.Add("properties", props); s.Add("additionalProperties", false);
