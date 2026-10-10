/*
 *  This file is part of nzbget. See <https://nzbget.com>.
 *
 *  Copyright (C) 2026 TheOtherP <theotherp@posteo.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "nzbget.h"

#include <boost/test/unit_test.hpp>
#include "QueueCoordinator.h"

BOOST_AUTO_TEST_SUITE(QueueTest)

static bool IsWait(time_t postTime, int delay, time_t now)
{
	return QueueCoordinator::IsPropagationWait(postTime, 0, delay, Options::pmFile, now);
}

static bool IsWaitAverage(time_t postTime, time_t nzbAvgTime, int delay, time_t now)
{
	return QueueCoordinator::IsPropagationWait(postTime, nzbAvgTime, delay, Options::pmAverage, now);
}

BOOST_AUTO_TEST_CASE(PropagationWaitDisabledTest)
{
	time_t now = 1760000000;

	BOOST_CHECK(!IsWait(now, 0, now));
	BOOST_CHECK(!IsWait(now + 60, 0, now));
	BOOST_CHECK(!IsWaitAverage(now, now, 0, now));
}

BOOST_AUTO_TEST_CASE(PropagationWaitTest)
{
	time_t now = 1760000000;
	int delay = 15 * 60;

	// posted just now or within the delay: held back
	BOOST_CHECK(IsWait(now, delay, now));
	BOOST_CHECK(IsWait(now - 14 * 60, delay, now));
	BOOST_CHECK(IsWait(now - delay, delay, now));

	// posted before the delay: downloadable
	BOOST_CHECK(!IsWait(now - delay - 1, delay, now));
	BOOST_CHECK(!IsWait(now - 3600, delay, now));
}

BOOST_AUTO_TEST_CASE(PropagationWaitPerFileTest)
{
	// a collection whose files were posted at different times:
	// the older file is downloadable, the late one (e.g. an nfo or a par2) is still held back
	time_t now = 1760000000;
	int delay = 15 * 60;
	time_t mkvPosted = now - 3600;
	time_t nfoPosted = now - 60;

	BOOST_CHECK(!IsWait(mkvPosted, delay, now));
	BOOST_CHECK(IsWait(nfoPosted, delay, now));
	BOOST_CHECK(!IsWait(nfoPosted, delay, nfoPosted + delay + 1));
}

BOOST_AUTO_TEST_CASE(PropagationWaitAverageTest)
{
	time_t now = 1760000000;
	int delay = 15 * 60;
	time_t nfoPosted = now - 60;

	// average post time older than the delay: nothing is held back, not even the late file
	BOOST_CHECK(!IsWaitAverage(nfoPosted, now - 3 * 3600, delay, now));
	BOOST_CHECK(!IsWaitAverage(now, now - delay - 1, delay, now));

	// average post time within the delay: the per-file check applies
	BOOST_CHECK(IsWaitAverage(nfoPosted, now - delay, delay, now));
	BOOST_CHECK(IsWaitAverage(nfoPosted, now - 5 * 60, delay, now));
	BOOST_CHECK(!IsWaitAverage(now - 3600, now - 5 * 60, delay, now));

	// unknown average (queue saved by an older version): the per-file check applies
	BOOST_CHECK(IsWaitAverage(nfoPosted, 0, delay, now));
	BOOST_CHECK(!IsWaitAverage(now - 3600, 0, delay, now));
}

BOOST_AUTO_TEST_CASE(PropagationWaitFileModeIgnoresAverageTest)
{
	time_t now = 1760000000;
	int delay = 15 * 60;

	BOOST_CHECK(QueueCoordinator::IsPropagationWait(now - 60, now - 3 * 3600, delay, Options::pmFile, now));
}

BOOST_AUTO_TEST_CASE(NzbAvgTimeTest)
{
	NzbInfo nzbInfo;
	BOOST_CHECK_EQUAL(nzbInfo.GetAvgTime(), 0);

	// times taken from a real nzb: six files posted together, release.nfo added ~22 hours later
	time_t times[] = { 1791537792, 1791537792, 1791537792, 1791537793, 1791537793, 1791537793, 1791615679 };
	for (time_t time : times)
	{
		std::unique_ptr<FileInfo> fileInfo = std::make_unique<FileInfo>();
		fileInfo->SetTime(time);
		nzbInfo.GetFileList()->Add(std::move(fileInfo));
	}

	// files without post time don't count
	nzbInfo.GetFileList()->Add(std::make_unique<FileInfo>());

	nzbInfo.UpdateMinMaxTime();

	BOOST_CHECK_EQUAL(nzbInfo.GetMinTime(), 1791537792);
	BOOST_CHECK_EQUAL(nzbInfo.GetMaxTime(), 1791615679);
	BOOST_CHECK_EQUAL(nzbInfo.GetAvgTime(), 1791548919);

	// when the nzb is added right after release.nfo was posted, the average is ~18.5 hours old,
	// so in average-mode release.nfo is not held back
	time_t added = 1791615679;
	BOOST_CHECK(IsWait(1791615679, 15 * 60, added));
	BOOST_CHECK(!IsWaitAverage(1791615679, nzbInfo.GetAvgTime(), 15 * 60, added));
}

BOOST_AUTO_TEST_SUITE_END()
