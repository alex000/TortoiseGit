// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2026 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "SyntaxHighlighter.h"

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <initializer_list>

#include "ILexer.h"
#include "Lexilla.h"
#include "SciLexer.h"

namespace
{

/////////////////////////////////////////////////////////////////////////////
// Language table

struct LanguageDef
{
	const wchar_t*	extensions;		///< space separated, lower case, with leading and trailing space
	const char*		lexer;			///< Lexilla lexer name
	const char*		keywords[4];	///< word lists 0..3
	const char*		properties;		///< "key=value;key=value"
};

constexpr const char* kCppKeywords =
	"alignas alignof and and_eq asm auto bitand bitor break case catch class compl concept const consteval constexpr "
	"constinit const_cast continue co_await co_return co_yield decltype default delete do dynamic_cast else enum explicit "
	"export extern false final for friend goto if inline mutable namespace new noexcept not not_eq nullptr operator or "
	"or_eq override private protected public register reinterpret_cast requires return sizeof static static_assert "
	"static_cast struct switch template this thread_local throw true try typedef typeid typename union using virtual "
	"volatile while xor xor_eq __declspec __forceinline __inline __stdcall __cdecl __fastcall __int64 __try __except __finally";
constexpr const char* kCppTypes =
	"bool char char8_t char16_t char32_t double float int long short signed unsigned void wchar_t size_t ptrdiff_t "
	"int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t intptr_t uintptr_t "
	"BOOL BYTE WORD DWORD DWORD_PTR UINT ULONG LONG INT_PTR UINT_PTR LPARAM WPARAM LRESULT HRESULT HANDLE HWND HDC "
	"LPSTR LPCSTR LPWSTR LPCWSTR LPTSTR LPCTSTR TCHAR WCHAR CHAR VOID PVOID LPVOID COLORREF CString CStringA CStringW";
constexpr const char* kDoxygen =
	"a addindex addtogroup anchor arg attention author b brief bug c class code date def defgroup deprecated dontinclude "
	"e em endcode endhtmlonly endif endlatexonly endlink endverbatim enum example exception f$ f[ f] file fn hideinitializer "
	"htmlinclude htmlonly if image include ingroup internal invariant interface latexonly li line link mainpage name "
	"namespace nosubgrouping note overload p page par param param[in] param[out] post pre ref relates remarks return retval "
	"sa section see showinitializer since skip skipline struct subsection test throw throws todo typedef union until var "
	"verbatim verbinclude version warning weakgroup";
constexpr const char* kCsKeywords =
	"abstract as async await base break case catch checked class const continue default delegate do else enum event explicit "
	"extern false finally fixed for foreach get goto if implicit in init interface internal is lock namespace new null "
	"operator out override params partial private protected public readonly record ref return sealed set sizeof stackalloc "
	"static struct switch this throw true try typeof unchecked unsafe using value var virtual volatile when where while yield";
constexpr const char* kCsTypes =
	"bool byte char decimal double dynamic float int long nint nuint object sbyte short string uint ulong ushort void";
constexpr const char* kJavaKeywords =
	"abstract assert break case catch class const continue default do else enum extends final finally for goto if implements "
	"import instanceof interface native new package private protected public return static strictfp super switch synchronized "
	"this throw throws transient try volatile while true false null var record sealed permits yield";
constexpr const char* kJavaTypes = "boolean byte char double float int long short void String Object";
constexpr const char* kJsKeywords =
	"abstract async await break case catch class const continue debugger default delete do else enum export extends false "
	"finally for from function get if implements import in instanceof interface let new null of package private protected "
	"public return set static super switch this throw true try typeof undefined var void while with yield as declare "
	"keyof namespace readonly type";
constexpr const char* kJsTypes = "any bigint boolean never number object string symbol unknown Array Map Set Promise";
constexpr const char* kGoKeywords =
	"break case chan const continue default defer else fallthrough for func go goto if import interface map package range "
	"return select struct switch type var true false nil iota";
constexpr const char* kGoTypes =
	"bool byte complex64 complex128 error float32 float64 int int8 int16 int32 int64 rune string uint uint8 uint16 uint32 "
	"uint64 uintptr any";
constexpr const char* kRcKeywords =
	"ACCELERATORS ALT AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON BEGIN BITMAP BLOCK BUTTON CAPTION CHARACTERISTICS CHECKBOX "
	"CLASS COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG DIALOGEX DISCARDABLE EDITTEXT END EXSTYLE FONT GROUPBOX ICON "
	"LANGUAGE LISTBOX LTEXT MENU MENUEX MENUITEM MESSAGETABLE POPUP PUSHBUTTON RADIOBUTTON RCDATA RTEXT SCROLLBAR SEPARATOR "
	"SHIFT STATE3 STRINGTABLE STYLE TEXTINCLUDE VALUE VERSION VERSIONINFO VIRTKEY";
constexpr const char* kPythonKeywords =
	"and as assert async await break case class continue def del elif else except False finally for from global if import "
	"in is lambda match None nonlocal not or pass raise return True try while with yield";
constexpr const char* kPythonBuiltins =
	"abs all any bool bytes callable chr dict dir enumerate filter float format getattr hasattr hash int isinstance "
	"issubclass iter len list map max min next object open ord print property range repr reversed round self set setattr "
	"slice sorted staticmethod str sum super tuple type zip";
constexpr const char* kRustKeywords =
	"as async await break const continue crate dyn else enum extern false fn for if impl in let loop match mod move mut pub "
	"ref return self Self static struct super trait true type unsafe use where while";
constexpr const char* kRustTypes =
	"bool char f32 f64 i8 i16 i32 i64 i128 isize str u8 u16 u32 u64 u128 usize String Vec Option Result Box";
constexpr const char* kPascalKeywords =
	"absolute abstract and array as asm assembler begin case cdecl class const constructor default destructor div do downto "
	"dynamic else end except exports external far file finalization finally for forward function goto if implementation in "
	"inherited initialization inline interface is label library message mod near nil not object of on or out overload "
	"override packed pascal private procedure program property protected public published raise record register "
	"reintroduce repeat resourcestring safecall set shl shr stdcall string then threadvar to try type unit until uses var "
	"virtual while with xor";
constexpr const char* kLuaKeywords =
	"and break do else elseif end false for function goto if in local nil not or repeat return then true until while";
constexpr const char* kBashKeywords =
	"alias bg bind break builtin case cd command continue declare do done echo elif else enable esac eval exec exit export "
	"false fc fg fi for function getopts hash help history if in jobs kill let local logout popd printf pushd pwd read "
	"readonly return select set shift source test then time trap true type typeset ulimit umask unalias unset until wait while";
constexpr const char* kBatchKeywords =
	"assoc break call cd chdir cls color copy date del dir do echo else endlocal equ erase errorlevel exist exit for geq goto "
	"gtr if in leq lss md mkdir move neq not nul path pause popd prompt pushd rd rem ren rename rmdir set setlocal shift "
	"start time title type ver verify vol";
constexpr const char* kSqlKeywords =
	"absolute action add after all alter and any as asc authorization begin between by cascade case cast check close "
	"collate column commit constraint create cross current cursor database declare default delete desc distinct drop else "
	"end escape except exec execute exists fetch for foreign from full function grant group having if in index inner "
	"insert intersect into is join key left like limit not null of offset on open or order outer primary procedure "
	"references return returns revoke right rollback select set table then to top transaction trigger truncate union "
	"unique update using values view when where while with";
constexpr const char* kSqlTypes =
	"bigint binary bit blob boolean char date datetime decimal double float int integer money nchar numeric nvarchar real "
	"smallint text time timestamp tinyint uniqueidentifier varbinary varchar xml";
constexpr const char* kPowerShellKeywords =
	"begin break catch class continue data default do dynamicparam else elseif end enum exit filter finally for foreach "
	"from function hidden if in param process return static switch throw trap try until using var while";
constexpr const char* kVbKeywords =
	"addhandler addressof alias and andalso as boolean byref byte byval call case catch cbool cbyte cchar cdate cdec cdbl "
	"char cint class clng cobj const continue csbyte cshort csng cstr ctype cuint culng cushort date decimal declare default "
	"delegate dim directcast do double each else elseif end enum erase error event exit false finally for friend function "
	"get gettype global gosub goto handles if implements imports in inherits integer interface is isnot let lib like long "
	"loop me mod module mustinherit mustoverride mybase myclass namespace narrowing new next not nothing notinheritable "
	"notoverridable object of on operator option optional or orelse overloads overridable overrides paramarray partial "
	"private property protected public raiseevent readonly redim rem removehandler resume return sbyte select set shadows "
	"shared short single static step stop string structure sub synclock then throw to true try trycast typeof uinteger "
	"ulong ushort using variant wend when while widening with withevents writeonly xor";

constexpr LanguageDef kLanguages[] = {
	{ L" c cc cpp cxx c++ h hh hpp hxx h++ inl ipp tcc ino cu cuh ", "cpp", { kCppKeywords, kCppTypes, kDoxygen, nullptr }, "lexer.cpp.track.preprocessor=0;lexer.cpp.escape.sequence=1" },
	{ L" rc rc2 dlg ", "cpp", { kRcKeywords, nullptr, nullptr, nullptr }, "lexer.cpp.track.preprocessor=0" },
	{ L" idl odl midl ", "cpp", { kCppKeywords, kCppTypes, nullptr, nullptr }, "lexer.cpp.track.preprocessor=0" },
	{ L" cs ", "cpp", { kCsKeywords, kCsTypes, kDoxygen, nullptr }, "lexer.cpp.track.preprocessor=0;lexer.cpp.verbatim.strings.allow.escapes=0" },
	{ L" java kt kts scala groovy gradle ", "cpp", { kJavaKeywords, kJavaTypes, kDoxygen, nullptr }, "lexer.cpp.track.preprocessor=0" },
	{ L" js jsx mjs cjs ts tsx ", "cpp", { kJsKeywords, kJsTypes, nullptr, nullptr }, "lexer.cpp.track.preprocessor=0;lexer.cpp.backquoted.strings=1;lexer.cpp.allow.dollars=1" },
	{ L" go ", "cpp", { kGoKeywords, kGoTypes, nullptr, nullptr }, "lexer.cpp.track.preprocessor=0;lexer.cpp.backquoted.strings=1" },
	{ L" swift dart ", "cpp", { kJsKeywords, nullptr, nullptr, nullptr }, "lexer.cpp.track.preprocessor=0" },
	{ L" json jsonc json5 geojson webmanifest ", "json", { "false true null", nullptr, nullptr, nullptr }, "lexer.json.allow.comments=1;lexer.json.escape.sequence=1" },
	{ L" py pyw pyi ", "python", { kPythonKeywords, kPythonBuiltins, nullptr, nullptr }, "" },
	{ L" rs ", "rust", { kRustKeywords, kRustTypes, nullptr, nullptr }, "" },
	{ L" pas dpr dpk pp lpr inc ", "pascal", { kPascalKeywords, nullptr, nullptr, nullptr }, "" },
	{ L" lua ", "lua", { kLuaKeywords, nullptr, nullptr, nullptr }, "" },
	{ L" sh bash zsh ksh ", "bash", { kBashKeywords, nullptr, nullptr, nullptr }, "" },
	{ L" bat cmd nt ", "batch", { kBatchKeywords, nullptr, nullptr, nullptr }, "" },
	{ L" ps1 psm1 psd1 ", "powershell", { kPowerShellKeywords, nullptr, nullptr, nullptr }, "" },
	{ L" sql ", "sql", { kSqlKeywords, kSqlTypes, nullptr, nullptr }, "" },
	{ L" vb vbs bas cls frm ", "vb", { kVbKeywords, nullptr, nullptr, nullptr }, "" },
	{ L" pl pm t ", "perl", { "if else elsif unless while until for foreach do sub my our local return last next redo use no require package and or not eq ne lt gt le ge cmp", nullptr, nullptr, nullptr }, "" },
	{ L" rb rake gemspec ", "ruby", { "alias and begin break case class def defined? do else elsif end ensure false for if in module next nil not or redo rescue retry return self super then true undef unless until when while yield", nullptr, nullptr, nullptr }, "" },
	{ L" html htm shtml xhtml htt hta htd php phtml asp aspx cshtml vue ", "hypertext", { nullptr, nullptr, nullptr, nullptr }, "" },
	{ L" xml xsl xslt xsd xaml svg wxs wxi wxl vcxproj csproj vbproj props targets filters resx config manifest plist nuspec ", "xml", { nullptr, nullptr, nullptr, nullptr }, "lexer.xml.allow.scripts=0" },
	{ L" yml yaml ", "yaml", { "true false yes no null", nullptr, nullptr, nullptr }, "" },
	{ L" ini inf reg cfg properties conf editorconfig gitconfig gitmodules gitattributes ", "props", { nullptr, nullptr, nullptr, nullptr }, "" },
	{ L" cmake ", "cmake", { nullptr, nullptr, nullptr, nullptr }, "" },
	{ L" mak mk ", "makefile", { nullptr, nullptr, nullptr, nullptr }, "" },
	{ L" diff patch rej ", "diff", { nullptr, nullptr, nullptr, nullptr }, "" },
	{ L" md markdown ", "markdown", { nullptr, nullptr, nullptr, nullptr }, "" },
	{ L" asm s ", "asm", { nullptr, nullptr, nullptr, nullptr }, "" },
};

/// files recognized by their name instead of their extension
constexpr struct
{
	const wchar_t* name;
	const char* lexer;
} kFileNames[] = {
	{ L"makefile", "makefile" },
	{ L"gnumakefile", "makefile" },
	{ L"cmakelists.txt", "cmake" },
	{ L".gitconfig", "props" },
	{ L".gitmodules", "props" },
	{ L".gitattributes", "props" },
	{ L".editorconfig", "props" },
};

std::wstring ToLower(std::wstring_view s)
{
	std::wstring ret(s);
	std::transform(ret.begin(), ret.end(), ret.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
	return ret;
}

std::wstring_view FileNamePart(std::wstring_view path)
{
	auto pos = path.find_last_of(L"\\/");
	if (pos != std::wstring_view::npos)
		path = path.substr(pos + 1);
	return path;
}

const LanguageDef* FindLanguage(std::wstring_view path, const char** pLexerByName)
{
	*pLexerByName = nullptr;
	const std::wstring name = ToLower(FileNamePart(path));
	if (name.empty())
		return nullptr;
	for (const auto& fn : kFileNames)
	{
		if (name == fn.name)
		{
			for (const auto& lang : kLanguages)
			{
				if (strcmp(lang.lexer, fn.lexer) == 0)
					return &lang;
			}
			*pLexerByName = fn.lexer;
			return nullptr;
		}
	}
	const auto dot = name.find_last_of(L'.');
	if (dot == std::wstring::npos || dot + 1 == name.size())
		return nullptr;
	const std::wstring ext = L" " + name.substr(dot + 1) + L" ";
	for (const auto& lang : kLanguages)
	{
		if (wcsstr(lang.extensions, ext.c_str()))
			return &lang;
	}
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// Style -> token class mapping

bool HasWord(const char* list, const char* word)
{
	if (!list)
		return false;
	const size_t len = strlen(word);
	for (const char* p = list; (p = strstr(p, word)) != nullptr; p += len)
	{
		const bool startOk = (p == list) || (p[-1] == ' ');
		const bool endOk = (p[len] == '\0') || (p[len] == ' ');
		if (startOk && endOk)
			return true;
	}
	return false;
}

/// maps the lexical class tags (see Lexilla's LexicalClass tables) of a style to a token class
SyntaxClass ClassFromTags(const char* tags, const char* name)
{
	if (!tags || !*tags)
		return SyntaxClass::Default;
	if (HasWord(tags, "comment"))
		return SyntaxClass::Comment;
	if (HasWord(tags, "literal"))
	{
		if (HasWord(tags, "numeric"))
			return SyntaxClass::Number;
		if (HasWord(tags, "regex"))
			return SyntaxClass::Regex;
		return SyntaxClass::String;
	}
	if (HasWord(tags, "tag"))
		return HasWord(tags, "operator") ? SyntaxClass::Default : SyntaxClass::Tag;
	if (HasWord(tags, "attribute"))
		return SyntaxClass::Attribute;
	if (HasWord(tags, "preprocessor"))
		return SyntaxClass::Preprocessor;
	if (HasWord(tags, "keyword"))
		return SyntaxClass::Keyword;
	if (HasWord(tags, "operator"))
		return SyntaxClass::Operator;
	if (HasWord(tags, "error"))
		return SyntaxClass::Error;
	if (HasWord(tags, "identifier") && name)
	{
		// secondary keyword lists are tagged as identifiers, e.g. SCE_C_WORD2, SCE_P_WORD2
		if (strstr(name, "WORD2") || strstr(name, "GLOBALCLASS") || strstr(name, "CLASSNAME") || strstr(name, "DEFNAME"))
			return SyntaxClass::Type;
	}
	return SyntaxClass::Default;
}

struct StyleMapEntry
{
	int style;
	SyntaxClass cls;
};

/// lexers which do not provide lexical class tags
void ManualStyleMap(const char* lexer, std::array<SyntaxClass, 256>& map)
{
	auto apply = [&map](std::initializer_list<StyleMapEntry> entries) {
		for (const auto& e : entries)
			map[e.style] = e.cls;
	};
	using C = SyntaxClass;
	if (strcmp(lexer, "json") == 0)
		apply({ { SCE_JSON_NUMBER, C::Number }, { SCE_JSON_STRING, C::String }, { SCE_JSON_STRINGEOL, C::String },
				{ SCE_JSON_PROPERTYNAME, C::Attribute }, { SCE_JSON_ESCAPESEQUENCE, C::String }, { SCE_JSON_LINECOMMENT, C::Comment },
				{ SCE_JSON_BLOCKCOMMENT, C::Comment }, { SCE_JSON_OPERATOR, C::Operator }, { SCE_JSON_URI, C::String },
				{ SCE_JSON_COMPACTIRI, C::String }, { SCE_JSON_KEYWORD, C::Keyword }, { SCE_JSON_LDKEYWORD, C::Keyword }, { SCE_JSON_ERROR, C::Error } });
	else if (strcmp(lexer, "yaml") == 0)
		apply({ { SCE_YAML_COMMENT, C::Comment }, { SCE_YAML_IDENTIFIER, C::Attribute }, { SCE_YAML_KEYWORD, C::Keyword },
				{ SCE_YAML_NUMBER, C::Number }, { SCE_YAML_REFERENCE, C::Type }, { SCE_YAML_DOCUMENT, C::Preprocessor },
				{ SCE_YAML_TEXT, C::String }, { SCE_YAML_ERROR, C::Error }, { SCE_YAML_OPERATOR, C::Operator } });
	else if (strcmp(lexer, "props") == 0)
		apply({ { SCE_PROPS_COMMENT, C::Comment }, { SCE_PROPS_SECTION, C::Tag }, { SCE_PROPS_ASSIGNMENT, C::Operator },
				{ SCE_PROPS_DEFVAL, C::Preprocessor }, { SCE_PROPS_KEY, C::Attribute } });
	else if (strcmp(lexer, "sql") == 0)
		apply({ { SCE_SQL_COMMENT, C::Comment }, { SCE_SQL_COMMENTLINE, C::Comment }, { SCE_SQL_COMMENTDOC, C::Comment },
				{ SCE_SQL_COMMENTLINEDOC, C::Comment }, { SCE_SQL_SQLPLUS_COMMENT, C::Comment }, { SCE_SQL_NUMBER, C::Number },
				{ SCE_SQL_WORD, C::Keyword }, { SCE_SQL_WORD2, C::Type }, { SCE_SQL_STRING, C::String }, { SCE_SQL_CHARACTER, C::String },
				{ SCE_SQL_OPERATOR, C::Operator }, { SCE_SQL_QUOTEDIDENTIFIER, C::String } });
	else if (strcmp(lexer, "powershell") == 0)
		apply({ { SCE_POWERSHELL_COMMENT, C::Comment }, { SCE_POWERSHELL_COMMENTSTREAM, C::Comment }, { SCE_POWERSHELL_COMMENTDOCKEYWORD, C::Comment },
				{ SCE_POWERSHELL_STRING, C::String }, { SCE_POWERSHELL_CHARACTER, C::String }, { SCE_POWERSHELL_HERE_STRING, C::String },
				{ SCE_POWERSHELL_HERE_CHARACTER, C::String }, { SCE_POWERSHELL_NUMBER, C::Number }, { SCE_POWERSHELL_VARIABLE, C::Attribute },
				{ SCE_POWERSHELL_OPERATOR, C::Operator }, { SCE_POWERSHELL_KEYWORD, C::Keyword }, { SCE_POWERSHELL_CMDLET, C::Type },
				{ SCE_POWERSHELL_ALIAS, C::Type }, { SCE_POWERSHELL_FUNCTION, C::Type } });
	else if (strcmp(lexer, "diff") == 0)
		apply({ { SCE_DIFF_COMMENT, C::Comment }, { SCE_DIFF_COMMAND, C::Keyword }, { SCE_DIFF_HEADER, C::Keyword },
				{ SCE_DIFF_POSITION, C::Preprocessor }, { SCE_DIFF_DELETED, C::Error }, { SCE_DIFF_ADDED, C::Comment },
				{ SCE_DIFF_CHANGED, C::Type } });
	else if (strcmp(lexer, "markdown") == 0)
		apply({ { SCE_MARKDOWN_STRONG1, C::Keyword }, { SCE_MARKDOWN_STRONG2, C::Keyword }, { SCE_MARKDOWN_EM1, C::Type },
				{ SCE_MARKDOWN_EM2, C::Type }, { SCE_MARKDOWN_HEADER1, C::Tag }, { SCE_MARKDOWN_HEADER2, C::Tag },
				{ SCE_MARKDOWN_HEADER3, C::Tag }, { SCE_MARKDOWN_HEADER4, C::Tag }, { SCE_MARKDOWN_HEADER5, C::Tag },
				{ SCE_MARKDOWN_HEADER6, C::Tag }, { SCE_MARKDOWN_ULIST_ITEM, C::Operator }, { SCE_MARKDOWN_OLIST_ITEM, C::Operator },
				{ SCE_MARKDOWN_BLOCKQUOTE, C::Comment }, { SCE_MARKDOWN_LINK, C::Attribute }, { SCE_MARKDOWN_CODE, C::String },
				{ SCE_MARKDOWN_CODE2, C::String }, { SCE_MARKDOWN_CODEBK, C::String } });
	else if (strcmp(lexer, "cmake") == 0)
		apply({ { SCE_CMAKE_COMMENT, C::Comment }, { SCE_CMAKE_STRINGDQ, C::String }, { SCE_CMAKE_STRINGLQ, C::String },
				{ SCE_CMAKE_STRINGRQ, C::String }, { SCE_CMAKE_COMMANDS, C::Keyword }, { SCE_CMAKE_PARAMETERS, C::Type },
				{ SCE_CMAKE_VARIABLE, C::Attribute }, { SCE_CMAKE_WHILEDEF, C::Keyword }, { SCE_CMAKE_FOREACHDEF, C::Keyword },
				{ SCE_CMAKE_IFDEFINEDEF, C::Keyword }, { SCE_CMAKE_MACRODEF, C::Keyword }, { SCE_CMAKE_STRINGVAR, C::Attribute },
				{ SCE_CMAKE_NUMBER, C::Number } });
}

/////////////////////////////////////////////////////////////////////////////
// UTF-8 helpers

/// number of UTF-8 bytes of the code point starting at s[i] (UTF-16), advances i past surrogate pairs
int Utf8LengthOfUtf16(std::wstring_view s, size_t& i)
{
	const unsigned int c = s[i];
	if (c < 0x80)
		return 1;
	if (c < 0x800)
		return 2;
	if (c >= 0xD800 && c <= 0xDBFF && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF)
	{
		++i; // the pair is one code point; both UTF-16 units get the same style
		return 4;
	}
	return 3;
}

void AppendUtf8(std::string& out, std::wstring_view s)
{
	for (size_t i = 0; i < s.size(); ++i)
	{
		unsigned int c = s[i];
		if (c >= 0xD800 && c <= 0xDBFF && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF)
		{
			c = 0x10000 + ((c - 0xD800) << 10) + (s[i + 1] - 0xDC00);
			++i;
		}
		else if (c >= 0xD800 && c <= 0xDFFF)
			c = 0xFFFD; // lone surrogate
		if (c < 0x80)
			out += static_cast<char>(c);
		else if (c < 0x800)
		{
			out += static_cast<char>(0xC0 | (c >> 6));
			out += static_cast<char>(0x80 | (c & 0x3F));
		}
		else if (c < 0x10000)
		{
			out += static_cast<char>(0xE0 | (c >> 12));
			out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (c & 0x3F));
		}
		else
		{
			out += static_cast<char>(0xF0 | (c >> 18));
			out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
			out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (c & 0x3F));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Minimal Scintilla document the lexers can work on

class CLexDocument : public Scintilla::IDocument
{
public:
	std::string			m_text;			///< UTF-8, lines separated by '\n'
	std::vector<Sci_Position> m_lineStarts;	///< byte offset of each line, plus a sentinel (length + 1)
	std::vector<char>	m_styles;
	std::vector<int>	m_lineStates;
	std::vector<int>	m_levels;
	Sci_Position		m_stylingPos = 0;

	void Init(int lineCount)
	{
		m_styles.assign(m_text.size(), 0);
		m_lineStates.assign(lineCount + 1, 0);
		m_levels.assign(lineCount + 1, 0x400); // SC_FOLDLEVELBASE
		m_stylingPos = 0;
	}
	Sci_Position LineCount() const { return static_cast<Sci_Position>(m_lineStarts.size()) - 1; }

	int SCI_METHOD Version() const override { return Scintilla::dvRelease4; }
	void SCI_METHOD SetErrorStatus(int) override {}
	Sci_Position SCI_METHOD Length() const override { return static_cast<Sci_Position>(m_text.size()); }
	void SCI_METHOD GetCharRange(char* buffer, Sci_Position position, Sci_Position lengthRetrieve) const override
	{
		if (position < 0 || lengthRetrieve <= 0)
			return;
		const Sci_Position len = std::min<Sci_Position>(lengthRetrieve, Length() - position);
		if (len > 0)
			memcpy(buffer, m_text.data() + position, static_cast<size_t>(len));
	}
	char SCI_METHOD StyleAt(Sci_Position position) const override
	{
		if (position < 0 || position >= static_cast<Sci_Position>(m_styles.size()))
			return 0;
		return m_styles[static_cast<size_t>(position)];
	}
	Sci_Position SCI_METHOD LineFromPosition(Sci_Position position) const override
	{
		if (position <= 0)
			return 0;
		auto it = std::upper_bound(m_lineStarts.cbegin(), m_lineStarts.cend() - 1, position);
		return std::max<Sci_Position>(0, static_cast<Sci_Position>(it - m_lineStarts.cbegin()) - 1);
	}
	Sci_Position SCI_METHOD LineStart(Sci_Position line) const override
	{
		if (line <= 0)
			return 0;
		if (line >= LineCount())
			return Length();
		return m_lineStarts[static_cast<size_t>(line)];
	}
	int SCI_METHOD GetLevel(Sci_Position line) const override
	{
		return (line >= 0 && line < static_cast<Sci_Position>(m_levels.size())) ? m_levels[static_cast<size_t>(line)] : 0x400;
	}
	int SCI_METHOD SetLevel(Sci_Position line, int level) override
	{
		if (line < 0 || line >= static_cast<Sci_Position>(m_levels.size()))
			return 0;
		const int old = m_levels[static_cast<size_t>(line)];
		m_levels[static_cast<size_t>(line)] = level;
		return old;
	}
	int SCI_METHOD GetLineState(Sci_Position line) const override
	{
		return (line >= 0 && line < static_cast<Sci_Position>(m_lineStates.size())) ? m_lineStates[static_cast<size_t>(line)] : 0;
	}
	int SCI_METHOD SetLineState(Sci_Position line, int state) override
	{
		if (line < 0 || line >= static_cast<Sci_Position>(m_lineStates.size()))
			return 0;
		const int old = m_lineStates[static_cast<size_t>(line)];
		m_lineStates[static_cast<size_t>(line)] = state;
		return old;
	}
	void SCI_METHOD StartStyling(Sci_Position position) override { m_stylingPos = position; }
	bool SCI_METHOD SetStyleFor(Sci_Position length, char style) override
	{
		if (length < 0 || m_stylingPos < 0)
			return false;
		const Sci_Position end = std::min<Sci_Position>(m_stylingPos + length, static_cast<Sci_Position>(m_styles.size()));
		for (Sci_Position i = m_stylingPos; i < end; ++i)
			m_styles[static_cast<size_t>(i)] = style;
		m_stylingPos += length;
		return true;
	}
	bool SCI_METHOD SetStyles(Sci_Position length, const char* styles) override
	{
		if (length < 0 || m_stylingPos < 0)
			return false;
		const Sci_Position end = std::min<Sci_Position>(m_stylingPos + length, static_cast<Sci_Position>(m_styles.size()));
		for (Sci_Position i = m_stylingPos; i < end; ++i)
			m_styles[static_cast<size_t>(i)] = styles[i - m_stylingPos];
		m_stylingPos += length;
		return true;
	}
	void SCI_METHOD DecorationSetCurrentIndicator(int) override {}
	void SCI_METHOD DecorationFillRange(Sci_Position, int, Sci_Position) override {}
	void SCI_METHOD ChangeLexerState(Sci_Position, Sci_Position) override {}
	int SCI_METHOD CodePage() const override { return 65001; } // SC_CP_UTF8
	bool SCI_METHOD IsDBCSLeadByte(char) const override { return false; }
	const char* SCI_METHOD BufferPointer() override { return m_text.c_str(); }
	int SCI_METHOD GetLineIndentation(Sci_Position line) override
	{
		int indent = 0;
		for (Sci_Position pos = LineStart(line), end = LineEnd(line); pos < end; ++pos)
		{
			const char ch = m_text[static_cast<size_t>(pos)];
			if (ch == ' ')
				++indent;
			else if (ch == '\t')
				indent = (indent / 8 + 1) * 8;
			else
				break;
		}
		return indent;
	}
	Sci_Position SCI_METHOD LineEnd(Sci_Position line) const override
	{
		if (line < 0)
			return 0;
		if (line >= LineCount() - 1)
			return Length();
		return m_lineStarts[static_cast<size_t>(line) + 1] - 1; // exclude the '\n'
	}
	Sci_Position SCI_METHOD GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const override
	{
		Sci_Position pos = positionStart;
		while (characterOffset > 0)
		{
			if (pos >= Length())
				return -1;
			Sci_Position width = 1;
			GetCharacterAndWidth(pos, &width);
			pos += width;
			--characterOffset;
		}
		while (characterOffset < 0)
		{
			if (pos <= 0)
				return -1;
			--pos;
			while (pos > 0 && (static_cast<unsigned char>(m_text[static_cast<size_t>(pos)]) & 0xC0) == 0x80)
				--pos;
			++characterOffset;
		}
		return pos;
	}
	int SCI_METHOD GetCharacterAndWidth(Sci_Position position, Sci_Position* pWidth) const override
	{
		if (position < 0 || position >= Length())
		{
			if (pWidth)
				*pWidth = 1;
			return 0;
		}
		const auto* p = reinterpret_cast<const unsigned char*>(m_text.data()) + position;
		const Sci_Position remaining = Length() - position;
		int width = 1;
		unsigned int ch = p[0];
		if (ch >= 0xF0 && remaining >= 4)
		{
			width = 4;
			ch = ((ch & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
		}
		else if (ch >= 0xE0 && remaining >= 3)
		{
			width = 3;
			ch = ((ch & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
		}
		else if (ch >= 0xC0 && remaining >= 2)
		{
			width = 2;
			ch = ((ch & 0x1F) << 6) | (p[1] & 0x3F);
		}
		if (pWidth)
			*pWidth = width;
		return static_cast<int>(ch);
	}
};

/////////////////////////////////////////////////////////////////////////////
// Lexer library

CSyntaxHighlighter::CreateLexerFn g_createLexer = nullptr;
bool g_lexerLibraryLoaded = false;

CSyntaxHighlighter::CreateLexerFn LoadLexerFactory()
{
	if (g_createLexer || g_lexerLibraryLoaded)
		return g_createLexer;
	g_lexerLibraryLoaded = true;
#ifdef _WIN32
	// ScintillaLexer_tgit.dll is installed next to TortoiseGitMerge.exe (it is used by TortoiseGitBlame and TortoiseGitUDiff)
	wchar_t path[MAX_PATH + 1] = { 0 };
	const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return nullptr;
	wchar_t* lastSlash = wcsrchr(path, L'\\');
	if (!lastSlash)
		return nullptr;
	*(lastSlash + 1) = L'\0';
	if (wcscat_s(path, L"ScintillaLexer_tgit.dll") != 0)
		return nullptr;
	HMODULE hLexer = LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hLexer)
		return nullptr;
	auto fn = reinterpret_cast<CSyntaxHighlighter::CreateLexerFn>(GetProcAddress(hLexer, LEXILLA_CREATELEXER));
	if (!fn)
		fn = reinterpret_cast<CSyntaxHighlighter::CreateLexerFn>(GetProcAddress(hLexer, "_CreateLexer@4")); // x86 stdcall decoration
	g_createLexer = fn;
#endif
	return g_createLexer;
}

} // namespace

/////////////////////////////////////////////////////////////////////////////
// CSyntaxHighlighter

void CSyntaxHighlighter::SetLexerFactory(CreateLexerFn fn)
{
	g_createLexer = fn;
	g_lexerLibraryLoaded = true;
}

CSyntaxHighlighter::~CSyntaxHighlighter()
{
	ReleaseLexer();
}

void CSyntaxHighlighter::ReleaseLexer()
{
	if (m_pLexer)
		m_pLexer->Release();
	m_pLexer = nullptr;
	m_runs.clear();
	m_lineRunStart.clear();
	m_bDirty = true;
}

const char* CSyntaxHighlighter::GetLexerNameForFile(std::wstring_view path)
{
	const char* lexerByName = nullptr;
	if (const LanguageDef* lang = FindLanguage(path, &lexerByName))
		return lang->lexer;
	return lexerByName;
}

bool CSyntaxHighlighter::SetLanguageForFile(std::wstring_view path)
{
	ReleaseLexer();
	const char* lexerByName = nullptr;
	const LanguageDef* lang = FindLanguage(path, &lexerByName);
	const char* lexerName = lang ? lang->lexer : lexerByName;
	if (!lexerName)
		return false;
	auto createLexer = LoadLexerFactory();
	if (!createLexer)
		return false;
	m_pLexer = createLexer(lexerName);
	if (!m_pLexer)
		return false;
	if (lang)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (lang->keywords[i])
				m_pLexer->WordListSet(i, lang->keywords[i]);
		}
		// "key=value;key=value"
		std::string props = lang->properties ? lang->properties : "";
		size_t start = 0;
		while (start < props.size())
		{
			size_t end = props.find(';', start);
			if (end == std::string::npos)
				end = props.size();
			const std::string item = props.substr(start, end - start);
			const size_t eq = item.find('=');
			if (eq != std::string::npos)
				m_pLexer->PropertySet(item.substr(0, eq).c_str(), item.substr(eq + 1).c_str());
			start = end + 1;
		}
	}
	BuildStyleMap(lexerName);
	m_bDirty = true;
	return true;
}

void CSyntaxHighlighter::BuildStyleMap(const char* lexerName)
{
	m_styleClass.fill(SyntaxClass::Default);
	if (!m_pLexer)
		return;
	const int named = m_pLexer->NamedStyles();
	if (named > 0)
	{
		for (int style = 0; style < std::min(named, 256); ++style)
			m_styleClass[style] = ClassFromTags(m_pLexer->TagsOfStyle(style), m_pLexer->NameOfStyle(style));
		// inactive (preprocessor disabled) styles of the C++ lexer are offset by 64
		if (strcmp(lexerName, "cpp") == 0)
		{
			for (int style = 0; style < 64 && style + 64 < 256; ++style)
				m_styleClass[style + 64] = SyntaxClass::Comment;
		}
	}
	else
		ManualStyleMap(lexerName, m_styleClass);
}

void CSyntaxHighlighter::Highlight(int lineCount, const std::function<std::wstring_view(int line)>& getLine)
{
	m_bDirty = false;
	m_runs.clear();
	m_lineRunStart.clear();
	if (!m_pLexer || lineCount <= 0)
		return;

	CLexDocument doc;
	doc.m_lineStarts.reserve(static_cast<size_t>(lineCount) + 1);
	std::vector<std::wstring_view> lines;
	lines.reserve(static_cast<size_t>(lineCount));
	size_t totalChars = 0;
	for (int i = 0; i < lineCount; ++i)
	{
		lines.push_back(getLine(i));
		totalChars += lines.back().size() + 1;
	}
	doc.m_text.reserve(totalChars + totalChars / 8);
	for (int i = 0; i < lineCount; ++i)
	{
		doc.m_lineStarts.push_back(static_cast<Sci_Position>(doc.m_text.size()));
		AppendUtf8(doc.m_text, lines[i]);
		if (i + 1 < lineCount)
			doc.m_text += '\n';
	}
	doc.m_lineStarts.push_back(static_cast<Sci_Position>(doc.m_text.size()) + 1);
	doc.Init(lineCount);

	m_pLexer->Lex(0, doc.Length(), 0, &doc);

	// convert the byte styles into runs of token classes per line, indexed by UTF-16 position
	m_runs.reserve(static_cast<size_t>(lineCount) * 4);
	m_lineRunStart.reserve(static_cast<size_t>(lineCount) + 1);
	for (int i = 0; i < lineCount; ++i)
	{
		m_lineRunStart.push_back(static_cast<int>(m_runs.size()));
		const std::wstring_view line = lines[i];
		size_t bytePos = static_cast<size_t>(doc.m_lineStarts[i]);
		SyntaxClass current = SyntaxClass::_Count; // forces a run at position 0
		for (size_t ch = 0; ch < line.size(); ++ch)
		{
			const size_t charStart = ch;
			const SyntaxClass cls = m_styleClass[static_cast<unsigned char>(doc.m_styles[bytePos])];
			bytePos += Utf8LengthOfUtf16(line, ch);
			if (cls != current)
			{
				m_runs.push_back({ static_cast<int>(charStart), cls });
				current = cls;
			}
		}
	}
	m_lineRunStart.push_back(static_cast<int>(m_runs.size()));
}

std::pair<const SyntaxRun*, const SyntaxRun*> CSyntaxHighlighter::GetRuns(int line) const
{
	if (line < 0 || line + 1 >= static_cast<int>(m_lineRunStart.size()))
		return { nullptr, nullptr };
	const SyntaxRun* base = m_runs.data();
	return { base + m_lineRunStart[line], base + m_lineRunStart[line + 1] };
}
