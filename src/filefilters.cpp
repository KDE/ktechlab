/***************************************************************************
 *   Copyright (C) 2023 by Friedrich W. H. Kossebau                        *
 *   kossebau@kde.org                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "filefilters.h"

QString FileFilters::toQtStyleString() const
{
    QStringList filters;
    for (const FileFilter &filter : *this) {
        filters.append(filter.name + QLatin1String(" (") + filter.patterns + QLatin1Char(')'));
    }
    return filters.join(QLatin1String(";;"));
}

QString FileFilters::toKDEStyleString() const
{
    QStringList filters;
    for (const FileFilter &filter : *this) {
        filters.append(filter.patterns + QLatin1Char('|') + filter.name);
    }
    return filters.join(QLatin1Char('\n'));
}
