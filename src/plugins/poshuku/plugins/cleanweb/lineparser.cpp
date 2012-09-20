/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "lineparser.h"
#include <QtDebug>
#include <boost/graph/graph_concepts.hpp>
#include "filter.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace CleanWeb
{
	LineParser::LineParser (Filter *f)
	: Filter_ (f)
	, Total_ (0)
	, Success_ (0)
	{
	}

	int LineParser::GetTotal () const
	{
		return Total_;
	}

	int LineParser::GetSuccess () const
	{
		return Success_;
	}

	void LineParser::operator() (const QString& line)
	{
		if (line.startsWith ('!'))
			return;

		++Total_;

		QString actualLine;
		FilterOption f = FilterOption ();
		bool cs = false;
		if (line.contains ('$'))
		{
			const auto& splitted = line.split ('$', QString::SkipEmptyParts);

			if (splitted.size () != 2)
			{
				qWarning () << Q_FUNC_INFO
					<< "incorrect usage of $-pattern:"
					<< splitted.size ()
					<< line;
				return;
			}

			actualLine = splitted.at (0);
			auto options = splitted.at (1).split (',', QString::SkipEmptyParts);

			if (options.removeAll ("match-case"))
			{
				f.Case_ = Qt::CaseSensitive;
				cs = true;
			}

			if (options.removeAll ("third-party"))
				f.AbortForeign_ = true;

			if (options.removeAll ("~third-party"))
				f.AbortForeign_ = false;

			Q_FOREACH (const QString& option, options)
				if (option.startsWith ("domain="))
				{
					const auto& domain = option.mid (7);
					if (domain.startsWith ('~'))
						f.NotDomains_ << domain.mid (1);
					else
						f.Domains_ << domain;
					options.removeAll (option);
				}

			if (options.size ())
			{
				qWarning () << Q_FUNC_INFO
						<< "unsupported options for filter"
						<< actualLine
						<< options;
				return;
			}
		}
		else
			actualLine = line;

		if (actualLine.contains ("##"))
		{
			const auto& split = actualLine.split ("##");
			if (split.size () != 2)
			{
				qWarning () << Q_FUNC_INFO
					<< "incorrect usage of ##-pattern:"
					<< split.size ()
					<< line;
				return;
			}

			actualLine = split.at (0);
			f.HideSelector_ = split.at (1);
		}

		bool white = false;
		if (actualLine.startsWith ("@@"))
		{
			actualLine.remove (0, 2);
			white = true;
		}

		if (actualLine.startsWith ('/') &&
				actualLine.endsWith ('/'))
		{
			actualLine = actualLine.mid (1, actualLine.size () - 2);
			f.MatchType_ = FilterOption::MTRegexp;
		}
		else
		{
			while (!actualLine.isEmpty () && actualLine.at (0) == '*')
				actualLine = actualLine.mid (1);
			while (!actualLine.isEmpty () && actualLine.at (actualLine.size () - 1) == '*')
				actualLine.chop (1);

			if (actualLine.startsWith ("||"))
			{
				auto spawned = "." + actualLine.mid (2);
				if (!cs)
					spawned = spawned.toLower ();

				f.MatchType_ = FilterOption::MTPlain;
				const FilterItem item { spawned, QRegExp (), QStringMatcher (spawned, f.Case_), f };
				if (white)
					Filter_->Exceptions_ << item;
				else
					Filter_->Filters_ << item;

				actualLine = actualLine.mid (2);
				actualLine.prepend ('/');
			}
			else if (actualLine.endsWith ('|') && actualLine.startsWith ('|'))
				actualLine = actualLine.mid (1, actualLine.size () - 2);
			else if (actualLine.endsWith ('|'))
			{
				actualLine.chop (1);
				if (actualLine.contains ('*'))
					actualLine.prepend ('*');
				else
					f.MatchType_ = FilterOption::MTEnd;
			}
			else if (actualLine.startsWith ('|'))
			{
				actualLine.remove (0, 1);
				if (actualLine.contains ('*'))
					actualLine.append ('*');
				else
					f.MatchType_ = FilterOption::MTBegin;
			}
			else
			{
				if (actualLine.contains ('*'))
				{
					actualLine.prepend ('*');
					actualLine.append ('*');
				}
				else
					f.MatchType_ = FilterOption::MTPlain;
			}

			if (f.MatchType_ == FilterOption::MTWildcard)
				actualLine.replace ('?', "\\?");
		}

		const auto& itemRx = f.MatchType_ == FilterOption::MTRegexp ?
				QRegExp (actualLine, f.Case_, QRegExp::RegExp) :
				QRegExp ();
		const QStringMatcher matcher = f.MatchType_ == FilterOption::MTPlain ?
				QStringMatcher (actualLine, f.Case_) :
				QStringMatcher ();
		const FilterItem item
		{
			(cs ? actualLine : actualLine.toLower ()),
			itemRx,
			matcher,
			f
		};

		if (white)
			Filter_->Exceptions_ << item;
		else
			Filter_->Filters_ << item;

		++Success_;
	}
}
}
}