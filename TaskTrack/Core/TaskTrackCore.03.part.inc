}


bool HasString(const Vector<String>& values, const String& value)
{
    for(const String& candidate : values)
        if(candidate == value)
            return true;
    return false;
}

bool UniqueStrings(const Vector<String>& values)
{
    Index<String> seen;
    for(const String& value : values) {
        if(seen.Find(value) >= 0)
            return false;
        seen.Add(value);
    }
    return true;
}

bool IsPositionToken(const String& value)
{
    static const char *values[] = {
        "top_left", "top", "top_right", "left", "center", "right",
        "bottom_left", "bottom", "bottom_right"
    };
    for(const char *candidate : values)
        if(value == candidate) return true;
    return false;
}

bool IsDirectionToken(const String& value)
{
    static const char *values[] = {
        "north_west", "north", "north_east", "west", "east",
        "south_west", "south", "south_east"
    };
    for(const char *candidate : values)
        if(value == candidate) return true;
    return false;
}

bool ValidateNumericDefault(TaskTrackItem& item, const String& prefix, String& error)
{
    if(IsNull(item.default_value))
        return true;
    if(!IsNumber(item.default_value)) {
        error = prefix + "default must be numeric.";
        return false;
    }
    double value = (double)item.default_value;
    if(item.has_min && value < item.min_value) {
        error = prefix + "default is below min.";
        return false;
    }
    if(item.has_max && value > item.max_value) {
        error = prefix + "default is above max.";
        return false;
    }
    return true;
}

bool ValidateItem(TaskTrackItem& item, const String& raw_type, const String& prefix, String& error)
{
    AddLegacyChoices(raw_type, item);

    if(item.title.IsEmpty()) {
        error = prefix + "title is required.";
        return false;
    }
    if(item.step_value <= 0.0) {
        error = prefix + "step must be greater than zero.";
        return false;
    }
    if(item.has_min && item.has_max && item.min_value > item.max_value) {
        error = prefix + "min must not exceed max.";
        return false;
    }
    if(!UniqueStrings(item.choices)) {
        error = prefix + "choices must be unique.";
        return false;
    }

    switch(item.type) {
    case TaskTrackItemType::Confirm:
        if(!item.choices.IsEmpty() && item.choices.GetCount() != 2) {
            error = prefix + "confirm accepts either no choices or exactly two display labels.";
            return false;
        }
        break;
    case TaskTrackItemType::SingleChoice:
    case TaskTrackItemType::MultiChoice:
    case TaskTrackItemType::Select:
    case TaskTrackItemType::ListSelect:
    case TaskTrackItemType::RankOrder:
        if(item.choices.IsEmpty()) {
            error = prefix + TaskTrackItemTypeName(item.type) + " requires choices.";
            return false;
        }
        break;
    case TaskTrackItemType::Number:
    case TaskTrackItemType::Amount:
        if(!ValidateNumericDefault(item, prefix, error))
            return false;
        break;
    case TaskTrackItemType::Range:
        if(!IsNull(item.default_value)) {
            if(!item.default_value.Is<ValueMap>()) {
                error = prefix + "range default must be {low,high}.";
                return false;
            }
            ValueMap def = item.default_value;
            if(!IsNumber(def["low"]) || !IsNumber(def["high"])) {
                error = prefix + "range default low/high must be numeric.";
                return false;
            }
            double low = (double)def["low"];
            double high = (double)def["high"];
            if(low > high || (item.has_min && low < item.min_value) ||
               (item.has_max && high > item.max_value)) {
                error = prefix + "range default must be ordered and inside min/max.";
                return false;
            }
        }
        break;
    case TaskTrackItemType::Rating:
        if(!item.has_min) { item.has_min = true; item.min_value = 1.0; }
        if(!item.has_max) { item.has_max = true; item.max_value = 5.0; }
        item.step_value = 1.0;
        if(item.max_value - item.min_value > 9.0 ||
           fabs(item.min_value - floor(item.min_value + 0.5)) > 0.000001 ||
           fabs(item.max_value - floor(item.max_value + 0.5)) > 0.000001) {
            error = prefix + "rating requires integer bounds spanning at most ten values.";
            return false;
        }
        break;
    case TaskTrackItemType::Color:
        if(!item.recommended.IsEmpty() && !ValidateColorText(item.recommended)) {
            error = prefix + "recommended color must be #RRGGBB or #RRGGBBAA.";
            return false;
        }
        break;
    case TaskTrackItemType::Gradient:
        if(item.gradients.IsEmpty()) {
            error = prefix + "gradient requires gradients.";
            return false;
        }
        if(!item.recommended.IsEmpty()) {
            bool found = false;
            for(const TaskTrackGradientOption& gradient : item.gradients)
                if(gradient.id == item.recommended) { found = true; break; }
            if(!found) {
                error = prefix + "recommended must match a gradient id.";
                return false;
            }
        }
        break;
    case TaskTrackItemType::Position:
        if(!IsNull(item.default_value) &&
           (!item.default_value.Is<String>() || !IsPositionToken(AsString(item.default_value)))) {
            error = prefix + "position default is invalid.";
            return false;
        }
        break;
    case TaskTrackItemType::Direction:
        if(!IsNull(item.default_value) &&
           (!item.default_value.Is<String>() || !IsDirectionToken(AsString(item.default_value)))) {
            error = prefix + "direction default is invalid.";
            return false;
        }
        break;
    case TaskTrackItemType::HierarchySelect:
        if(item.hierarchy.IsEmpty()) {
            error = prefix + "hierarchy_select requires hierarchy nodes.";
            return false;
        }
        break;
    case TaskTrackItemType::Curve:
        if(!IsNull(item.default_value)) {
            if(!item.default_value.Is<ValueArray>()) {
                error = prefix + "curve default must be [x1,y1,x2,y2].";
                return false;
            }
            ValueArray points = item.default_value;
            if(points.GetCount() != 4) {
                error = prefix + "curve default must contain four values.";
                return false;
            }
            for(int i = 0; i < 4; ++i) {
                if(!IsNumber(points[i]) || (double)points[i] < 0.0 || (double)points[i] > 1.0) {
                    error = prefix + "curve default values must be numbers from 0 to 1.";
                    return false;
                }
            }
        }
        break;
    default:
        break;
    }

    if(!item.recommended.IsEmpty()) {
        bool discrete = item.type == TaskTrackItemType::SingleChoice ||
                        item.type == TaskTrackItemType::Select ||
                        item.type == TaskTrackItemType::ListSelect;
        if(discrete && !HasString(item.choices, item.recommended)) {
            error = prefix + "recommended must match one of choices.";
            return false;
        }
    }

    return true;
}

bool ParseItem(const Value& raw, TaskTrackItem& item, const String& prefix, String& error)
{
    if(!raw.Is<ValueMap>()) {
        error = prefix + "must be an object.";
        return false;
