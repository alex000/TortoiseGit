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
#pragma once

#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace Scintilla
{
class ILexer5;
}

/**
 * \ingroup TortoiseMerge
 * Generic token classes. The (many) lexer specific styles of the Lexilla
 * lexers are mapped to these, so that only one small palette per theme is
 * needed for all languages.
 */
enum class SyntaxClass : unsigned char
{
	Default,
	Comment,
	Keyword,
	Type,
	String,
	Number,
	Preprocessor,
	Operator,
	Regex,
	Tag,
	Attribute,
	Error,
	_Count
};

/// a run of characters with the same token class, starting at \a start (UTF-16 index into the line)
struct SyntaxRun
{
	int start;
	SyntaxClass cls;
};

/**
 * \ingroup TortoiseMerge
 * Syntax highlighting for a view, based on the Lexilla lexers which are
 * already shipped with TortoiseGit (ScintillaLexer_tgit.dll, used by
 * TortoiseGitBlame and TortoiseGitUDiff).
 *
 * The whole document is lexed at once (Lexilla lexers are fast: a 12000
 * lines C++ file takes a few milliseconds) and the result is stored as runs
 * of token classes per line. The highlighter does not know about views or
 * diff states; it is fed with plain lines via Highlight().
 */
class CSyntaxHighlighter
{
public:
#ifdef _WIN32
	using CreateLexerFn = Scintilla::ILexer5*(__stdcall*)(const char* name);
#else
	using CreateLexerFn = Scintilla::ILexer5* (*)(const char* name);
#endif
	/// Overrides the lexer factory (default: CreateLexer from ScintillaLexer_tgit.dll). Used by tests.
	static void SetLexerFactory(CreateLexerFn fn);

	CSyntaxHighlighter() = default;
	~CSyntaxHighlighter();
	CSyntaxHighlighter(const CSyntaxHighlighter&) = delete;
	CSyntaxHighlighter& operator=(const CSyntaxHighlighter&) = delete;

	/**
	 * Selects the language from the extension (or name) of \a path.
	 * \return true if a lexer is available for this file type
	 */
	bool SetLanguageForFile(std::wstring_view path);
	/// the Lexilla lexer name for the file, or nullptr if the file type is not supported
	static const char* GetLexerNameForFile(std::wstring_view path);

	bool IsActive() const { return m_pLexer != nullptr; }
	/// marks the highlighting as outdated, e.g. after an edit
	void Invalidate() { m_bDirty = true; }
	bool IsDirty() const { return m_bDirty; }

	/**
	 * Lexes the document.
	 * \param lineCount number of lines
	 * \param getLine returns the text of a line (without line ending)
	 */
	void Highlight(int lineCount, const std::function<std::wstring_view(int line)>& getLine);

	/// runs for \a line, ordered by start. Returns an empty range if not highlighted.
	std::pair<const SyntaxRun*, const SyntaxRun*> GetRuns(int line) const;
	int GetLineCount() const { return m_lineRunStart.empty() ? 0 : static_cast<int>(m_lineRunStart.size()) - 1; }

	/// maps a lexer style to a token class (exposed for tests)
	SyntaxClass ClassForStyle(int style) const { return m_styleClass[style & 0xFF]; }

private:
	void ReleaseLexer();
	void BuildStyleMap(const char* lexerName);

	Scintilla::ILexer5*					m_pLexer = nullptr;
	std::array<SyntaxClass, 256>		m_styleClass{};
	bool								m_bDirty = true;
	std::vector<SyntaxRun>				m_runs;
	std::vector<int>					m_lineRunStart; ///< index into m_runs per line, plus one sentinel
};
