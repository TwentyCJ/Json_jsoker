#pragma once
#include <map>
#include <vector>
#include <string>
#include <stack>
#include <queue>
using namespace std;
#pragma region JSON对象定义
//JSON对象抽象基类
class Jreader
{
public:
	virtual void push_jarray(wchar_t *str) = 0;
	virtual void push_jobject(Jreader *object) = 0;
};
//封装整个JSON对象
class Jobject :public Jreader
{
public:
	~Jobject() {
		map<wchar_t *, wchar_t *>::iterator oiter;
		for (oiter = Objects.begin(); oiter != Objects.end(); ++oiter)
			delete oiter->first, oiter->second;
		vector<pair<wchar_t *, Jreader *>>::iterator aiter;
		for (aiter = Arrays.begin(); aiter != Arrays.end(); ++aiter)
			delete aiter->first, aiter->second;
	}
	void push_jarray(wchar_t *str) {}
	void push_jobject(Jreader *object) {}
	void push_jarray(wchar_t *str, wchar_t *str1) { Objects[str] = str1; }
	void push_jobject(wchar_t *str, Jreader *object) { Arrays.push_back(make_pair(str, object)); }
	Jreader *End_array() {
		vector<pair<wchar_t *, Jreader *>>::iterator iter;
		iter = Arrays.end();
		--iter;
		return iter->second;
	}
private:
	map<wchar_t *, wchar_t *>Objects;
	vector<pair<wchar_t *, Jreader *>>Arrays;
};
//封装JSON数组
class Jarray :public Jreader
{
public:
	~Jarray() {
		vector<wchar_t *>::iterator iter;
		for (iter = Arrays.begin(); iter != Arrays.end(); ++iter)
			delete *iter;
	}
	void push_jarray(wchar_t *str) { Arrays.push_back(str); }
	void push_jobject(Jreader *object) {}
private:
	vector<wchar_t *>Arrays;
};
//封装JSON数组内部对象 
class JAobject :public Jreader
{
public:
	~JAobject() {
		vector<Jreader *>::iterator iter;
		for (iter = Arrays.begin(); iter != Arrays.end(); ++iter)
			delete *iter;
	}
	void push_jarray(wchar_t *str) {}
	void push_jobject(Jreader *object) { Arrays.push_back(object); }
private:
	vector<Jreader *>Arrays;
};
#pragma endregion

#pragma region 状态抽象系状态机基类 
//状态枚举 
typedef enum JState { FREE_S, KEY_S, VALUE_S, ARRAY_S };
//状态机状态抽象基类 
class Json_State
{
public:
	virtual void Analysis(wchar_t &Wchar) = 0;
};
//状态机抽象基类
class Json_Analysis
{
public:
	virtual Jobject *Analysis(wchar_t *str, int len) = 0;
	virtual void Set_state(JState s) {}
	virtual void Push_char(wchar_t &str) {}
	virtual void Pop_char() {}
	virtual void Push_catch() {}
	virtual bool Pop_catch() = 0;
	virtual void New_array() {}
	virtual void Pop_array() {}
	virtual void Next_catch() {}
	virtual void Next_key() {}
};
#pragma endregion 

#pragma region 状态与状态机实现
//空闲状态 
class Free_State : public Json_State
{
public:
	Free_State(Json_Analysis *reader) { Reader = reader; }
	void Analysis(wchar_t &Wchar) {
		if (Wchar == L'{')
			Reader->Set_state(KEY_S);
	}
private:
	Json_Analysis *Reader;
};
//键状态 
class Key_State : public Json_State
{
public:
	Key_State(Json_Analysis *reader) { Reader = reader; }
	void Analysis(wchar_t &Wchar) {
		if (Wchar == L':') {
			Reader->Pop_char();
			Reader->Set_state(VALUE_S);
			return;
		}Reader->Push_char(Wchar);
	}
private:
	Json_Analysis *Reader;
};
//值状态 
class Value_State : public Json_State
{
public:
	Value_State(Json_Analysis *reader) { Reader = reader; }
	void Analysis(wchar_t &Wchar) {
		switch (Wchar)
		{
		case L',':
			if (*(&Wchar + 1) == L'{')
				return;
			if (*(&Wchar - 1) == L']') {
				Reader->Set_state(KEY_S);
				return;
			}
			Reader->Next_key();
			Reader->Set_state(KEY_S);
			return;
		case L'[':
			if (*(&Wchar + 1) == L'{') {
				Reader->Push_catch();
				return;
			}
			Reader->New_array();
			Reader->Set_state(ARRAY_S);
			return;
		case L'{':
			Reader->Next_catch();
			Reader->Set_state(KEY_S);
			return;
		case L'}':
			if (*(&Wchar - 1) != L']')
				Reader->Next_key();
			if (!Reader->Pop_catch())
				Reader->Set_state(FREE_S);
			return;
		case L']':
			return;
		default:
			Reader->Push_char(Wchar);
			break;
		}
	}
private:
	Json_Analysis *Reader;
};
//数组状态 
class Array_State : public Json_State
{
public:
	Array_State(Json_Analysis *reader) { Reader = reader; }
	void Analysis(wchar_t &Wchar) {
		if (Wchar == L',') {
			Reader->Pop_array();
			return;
		}
		if (Wchar == L']') {
			Reader->Pop_array();
			Reader->Set_state(VALUE_S);
			return;
		}Reader->Push_char(Wchar);
	}
private:
	Json_Analysis *Reader;
};
//JSON状态机 
class Json_Reader :public Json_Analysis
{
public:
	Json_Reader() {
		FREE_SP = new Free_State(this);
		KEY_SP = new Key_State(this);
		VALUE_SP = new Value_State(this);
		ARRAY_SP = new Array_State(this);
		STATE = FREE_SP;
	}
	~Json_Reader() { delete FREE_SP, KEY_SP, VALUE_SP, ARRAY_SP; }
	Jobject *Analysis(wchar_t *str, int len) {
		Object_Catch = new Jobject;
		for (int i = 0; i < len; ++i)
			STATE->Analysis(*str);
		return Object_Catch;
	}
	void Set_state(JState s) {
		switch (s)
		{
		case FREE_S:STATE = FREE_SP;
			break;
		case KEY_S:STATE = KEY_SP;
			break;
		case VALUE_S:STATE = VALUE_SP;
			break;
		case ARRAY_S:STATE = ARRAY_SP;
			break;
		default:
			break;
		}
	}
	void Push_char(wchar_t &str) { Char_Catch.push(str); }
	void Pop_char() {
		int len = Char_Catch.size();
		Str_Catch = new wchar_t[len + 1];
		Str_Catch[len] = L'\0';
		for (int i = 0; i < len; ++i) {
			Str_Catch[i] = Char_Catch.front();
			Char_Catch.pop();
		}
	}
	void Push_catch() {
		Jreader *temp = new JAobject;
		Object_Catch->push_jobject(Str_Catch, temp);
	}
	bool Pop_catch() {
		if (Stack_Object_Catch.size()) {
			Object_Catch = Stack_Object_Catch.top();
			Stack_Object_Catch.pop(); return true;
		}return false;
	}
	void New_array() {
		Array_Catch = new Jarray;
		Object_Catch->push_jobject(Str_Catch, Array_Catch);
	}
	void Pop_array() {
		Pop_char();
		Array_Catch->push_jarray(Str_Catch);
	}
	void Next_catch() {
		Jobject *object = new Jobject;
		Jreader *temp = Object_Catch->End_array();
		temp->push_jobject(object);
		Stack_Object_Catch.push(Object_Catch);
		Object_Catch = object;
	}
	void Next_key() {
		wchar_t *temp = Str_Catch;
		Pop_char();
		Object_Catch->push_jarray(temp, Str_Catch);
	}
private:
	wchar_t *Str_Catch;
	Jobject *Object_Catch;
	Jarray *Array_Catch;
	queue<wchar_t> Char_Catch;
	stack<Jobject *> Stack_Object_Catch;
	Json_State *STATE;
	Json_State *FREE_SP, *KEY_SP, *VALUE_SP, *ARRAY_SP;
};
#pragma endregion 