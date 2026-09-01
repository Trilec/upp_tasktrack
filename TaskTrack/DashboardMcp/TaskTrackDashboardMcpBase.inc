static const char *CURRENT_PROTOCOL="2026-07-28";
String ReadString(const Value& m,const char* k,const String& d=String()){Value v=m[k];return IsNull(v)?d:AsString(v);}
bool ReadBool(const Value& m,const char* k,bool d,bool& out,String& e){Value v=m[k];if(IsNull(v)){out=d;return true;}if(!v.Is<bool>()){e=String(k)+" must be a boolean.";return false;}out=(bool)v;return true;}
bool ReadInt(const Value& m,const char* k,int d,int& out,String& e){Value v=m[k];if(IsNull(v)){out=d;return true;}if(!IsNumber(v)){e=String(k)+" must be numeric.";return false;}out=(int)v;return true;}
Value ServerInfoValue(){ValueMap x;x.Add("name","tasktrack_dashboard_mcp");x.Add("version",TaskTrackDashboardVersion());x.Add("description","Durable semantic project dashboards for AI-maintained project state.");return x;}
String RequestProtocol(const Value& r){Value p=r["params"];if(!p.Is<ValueMap>())return String();Value m=p["_meta"];if(!m.Is<ValueMap>())return String();return ReadString(m,"io.modelcontextprotocol/protocolVersion");}
bool IsModern(const Value& r){return RequestProtocol(r)==CURRENT_PROTOCOL;}
void AddModernMeta(ValueMap& r){ValueMap m;m.Add("io.modelcontextprotocol/serverInfo",ServerInfoValue());r.Add("_meta",m);}
Value JsonRpcResult(const Value& id,const Value& result){ValueMap r;r.Add("jsonrpc","2.0");r.Add("id",id);r.Add("result",result);return r;}
Value JsonRpcError(const Value& id,int code,const String& msg){ValueMap e;e.Add("code",code);e.Add("message",msg);ValueMap r;r.Add("jsonrpc","2.0");r.Add("id",id);r.Add("error",e);return r;}
Value ToolText(const String& text){ValueMap b;b.Add("type","text");b.Add("text",text);return b;}
Value BuildCallToolResult(const Value& structured,bool is_error,bool modern){ValueMap r;if(modern)r.Add("resultType","complete");ValueArray c;c.Add(ToolText(AsJSON(structured,true)));r.Add("content",c);r.Add("structuredContent",structured);if(is_error)r.Add("isError",true);if(modern)AddModernMeta(r);return r;}
Value Failure(const String& code,const String& msg){ValueMap x;x.Add("ok",false);x.Add("code",code);x.Add("message",msg);return x;}
Value Schema(const String& t,const String& d=String()){ValueMap s;if(!t.IsEmpty())s.Add("type",t);if(!d.IsEmpty())s.Add("description",d);return s;}
Value StringSchema(const String& d=String()){return Schema("string",d);} Value BoolSchema(const String& d=String()){return Schema("boolean",d);} Value NumberSchema(const String& d=String()){return Schema("number",d);}
Value IntSchema(int mn,int mx,const String& d=String()){ValueMap s;s.Add("type","integer");s.Add("minimum",mn);s.Add("maximum",mx);if(!d.IsEmpty())s.Add("description",d);return s;}
Value EnumStringSchema(const char** v,int n,const String& d){ValueArray a;for(int i=0;i<n;++i)a.Add(v[i]);ValueMap s;s.Add("type","string");s.Add("enum",a);s.Add("description",d);return s;}
Value ToolAnnotations(bool ro,bool destructive,bool idem){ValueMap a;a.Add("readOnlyHint",ro);a.Add("destructiveHint",destructive);a.Add("idempotentHint",idem);return a;}
Value ToolSpec(const String& name,const String& desc,const Value& input,bool ro,bool destructive,bool idem){ValueMap t;t.Add("name",name);t.Add("description",desc);t.Add("inputSchema",input);t.Add("annotations",ToolAnnotations(ro,destructive,idem));return t;}
Value EmptyObjectSchema(){ValueMap s;s.Add("type","object");s.Add("properties",ValueMap());s.Add("additionalProperties",false);return s;}
