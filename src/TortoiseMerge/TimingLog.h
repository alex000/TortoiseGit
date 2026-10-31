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

/**
 * Lightweight load-time instrumentation.
 *
 * Disabled by default and costs a single (cached) bool check per scope.
 * Enable by setting the environment variable TGM_TIMING=1 before starting
 * TortoiseGitMerge. Timings are appended to %TEMP%\TortoiseGitMerge-timing.log
 * and also sent to OutputDebugString (visible in DebugView).
 *
 * Each line contains the time since process start, the duration of the
 * scope and the nesting depth, e.g.
 *     [  412.3 ms]   1234.5 ms   CDiffData::Load
 */
class CTimingLog
{
public:
	static bool Enabled()
	{
		static const bool enabled = []() {
			wchar_t buf[8] = { 0 };
			return GetEnvironmentVariableW(L"TGM_TIMING", buf, _countof(buf)) > 0 && buf[0] != L'0';
		}();
		return enabled;
	}

	static double NowMs()
	{
		static const double freq = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return static_cast<double>(f.QuadPart); }();
		LARGE_INTEGER c;
		QueryPerformanceCounter(&c);
		return static_cast<double>(c.QuadPart) * 1000.0 / freq;
	}

	/// milliseconds since the process was created
	static double SinceProcessStartMs()
	{
		static const double offset = []() {
			FILETIME ftCreate, ftExit, ftKernel, ftUser, ftNow;
			if (!GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser))
				return NowMs();
			GetSystemTimeAsFileTime(&ftNow);
			ULARGE_INTEGER c, n;
			c.LowPart = ftCreate.dwLowDateTime; c.HighPart = ftCreate.dwHighDateTime;
			n.LowPart = ftNow.dwLowDateTime; n.HighPart = ftNow.dwHighDateTime;
			double elapsed = static_cast<double>(n.QuadPart - c.QuadPart) / 10000.0; // 100ns -> ms
			return NowMs() - elapsed;
		}();
		return NowMs() - offset;
	}

	static int& Depth() { static int depth = 0; return depth; }

	static void Write(const char* name, double durationMs, int depth)
	{
		char line[512];
		sprintf_s(line, "[%9.1f ms] %9.1f ms %*s%s\n", SinceProcessStartMs(), durationMs, depth * 2, "", name);
		OutputDebugStringA(line);
		wchar_t path[MAX_PATH + 1] = { 0 };
		if (!GetTempPathW(MAX_PATH - 32, path))
			return;
		wcscat_s(path, L"TortoiseGitMerge-timing.log");
		FILE* f = nullptr;
		if (_wfopen_s(&f, path, L"a") == 0 && f)
		{
			fputs(line, f);
			fclose(f);
		}
	}

	static void Mark(const char* name)
	{
		if (Enabled())
			Write(name, 0.0, Depth());
	}
};

class CScopedTiming
{
public:
	explicit CScopedTiming(const char* name)
		: m_name(name)
	{
		if (CTimingLog::Enabled())
		{
			m_start = CTimingLog::NowMs();
			++CTimingLog::Depth();
		}
	}
	~CScopedTiming()
	{
		if (CTimingLog::Enabled())
		{
			--CTimingLog::Depth();
			CTimingLog::Write(m_name, CTimingLog::NowMs() - m_start, CTimingLog::Depth());
		}
	}
	CScopedTiming(const CScopedTiming&) = delete;
	CScopedTiming& operator=(const CScopedTiming&) = delete;

private:
	const char* m_name;
	double m_start = 0.0;
};

#define TGM_TIMING_CONCAT2(a, b) a##b
#define TGM_TIMING_CONCAT(a, b) TGM_TIMING_CONCAT2(a, b)
/// times the enclosing scope
#define TGM_TIMING(name) CScopedTiming TGM_TIMING_CONCAT(tgmTiming_, __LINE__)(name)
/// times the enclosing scope, using the function name
#define TGM_TIMING_FUNC() TGM_TIMING(__FUNCTION__)
