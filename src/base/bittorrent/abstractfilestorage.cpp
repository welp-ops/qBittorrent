/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2020  Vladimir Golovnev <glassez@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "abstractfilestorage.h"

#include <QDir>
#include <QHash>
#include <QSet>
#include <QVector>

#include "base/bittorrent/common.h"
#include "base/exceptions.h"
#include "base/utils/fs.h"

#if defined(Q_OS_WIN)
const Qt::CaseSensitivity CASE_SENSITIVITY {Qt::CaseInsensitive};
#else
const Qt::CaseSensitivity CASE_SENSITIVITY {Qt::CaseSensitive};
#endif

QVector<int> BitTorrent::AbstractFileStorage::folderIndexes(const QString &folder) const
{
    QVector<int> result;

    for (int i = 0; i < filesCount(); ++i)
    {
        if (filePath(i).startsWith(folder, CASE_SENSITIVITY))
            result.append(i);
    }

    return result;
}

void BitTorrent::AbstractFileStorage::renameFiles(const QVector<int> &indexes
                                                  , const QVector<QString> &newNames)
{
    // both these are without QB_EXT:
    QSet<QString> oldAndFolderNames;
    for (int i = 0; i < filesCount(); i++)
    {
        if (!indexes.contains(i))
        {
            oldAndFolderNames.insert(Utils::Fs::stripQbExtension(filePath(i)));
        }

        for (const QString &folder : Utils::Fs::parentFolders(filePath(i)))
        {
            oldAndFolderNames.insert(folder);
        }
    }

    for (auto it = newNames.cbegin(); it != newNames.cend(); it++) {
        if (oldAndFolderNames.contains(*it)) {
            throw RuntimeError {tr("Renamed file would conflict with other file in torrent: '%1'.").arg(*it)};
        }
        if (std::find(it+1, newNames.cend(), *it) != newNames.cend()) {
            throw RuntimeError {tr("Multiple newly renamed files would have the same name: '%1'.").arg(*it)};
        }
        // TODO: prevent overwriting files. Have to see if libtorrent already takes any renaming overwrite precautions.
        // TODO: handle .. in file names
        // TODO: at what point are backslashes turned into slashes? Are they forward slashes in libtorrent?
    }

    for (int i = 0; i < indexes.size(); i++) {
        const QString newNameWithExtension = Utils::Fs::hasQbExtension(filePath(indexes[i]))
            ? Utils::Fs::ensureQbExtension(newNames[i])
            : newNames[i];
        renameFile(indexes[i], newNameWithExtension);
    }
}

void BitTorrent::AbstractFileStorage::renameFile(const QString &oldPath, const QString &newPath)
{
    if (!Utils::Fs::isValidFileSystemName(oldPath, true))
        throw RuntimeError {tr("The old path is invalid: '%1'.").arg(oldPath)};

    const QString oldFilePath = Utils::Fs::toUniformPath(oldPath);
    if (oldFilePath.endsWith(QLatin1Char {'/'}))
        throw RuntimeError {tr("Invalid file path: '%1'.").arg(oldFilePath)};

    int renamingFileIndex = -1;
    for (int i = 0; i < filesCount(); ++i)
    {
        const QString path = filePath(i);

        if ((renamingFileIndex < 0) && Utils::Fs::stripQbExtension(path) == Utils::Fs::stripQbExtension(oldFilePath))
            renamingFileIndex = i;
    }

    if (renamingFileIndex < 0)
        throw RuntimeError {tr("No such file: '%1'.").arg(oldFilePath)};

    renameFileChecked(renamingFileIndex, newPath);
}

void BitTorrent::AbstractFileStorage::renameFileChecked(int index, const QString &newPath)
{
    if (!Utils::Fs::isValidFileSystemName(newPath, true))
        throw RuntimeError {tr("The new path is invalid: '%1'.").arg(newPath)};

    const QString newFilePath = Utils::Fs::toUniformPath(newPath);
    if (newFilePath.endsWith(QLatin1Char {'/'}))
        throw RuntimeError {tr("Invalid file path: '%1'.").arg(newFilePath)};

    renameFiles(QVector<int>{index}, QVector<QString>{newFilePath});
}

void BitTorrent::AbstractFileStorage::renameFolder(const QString &oldPath, const QString &newPath)
{
    if (!Utils::Fs::isValidFileSystemName(oldPath, true))
        throw RuntimeError {tr("The old path is invalid: '%1'.").arg(oldPath)};
    if (!Utils::Fs::isValidFileSystemName(newPath, true))
        throw RuntimeError {tr("The new path is invalid: '%1'.").arg(newPath)};

    const auto cleanFolderPath = [](const QString &path) -> QString
    {
        const QString uniformPath = Utils::Fs::toUniformPath(path);
        return (uniformPath.endsWith(QLatin1Char {'/'}) ? uniformPath : uniformPath + QLatin1Char {'/'});
    };

    const QString oldFolderPath = cleanFolderPath(oldPath);
    const QString newFolderPath = cleanFolderPath(newPath);

    QVector<int> renamingFileIndexes = folderIndexes(oldFolderPath);

    if (renamingFileIndexes.isEmpty())
        throw RuntimeError {tr("No such folder: '%1'.").arg(oldFolderPath)};

    QVector<QString> renamingFilePaths;
    for (int idx : renamingFileIndexes) {
        renamingFilePaths.push_back(filePath(idx));
    }
    std::function<QString (const QString &)> nameTransformer;
    nameTransformer = [newFolderPath, oldFolderPath](const QString &filePath) -> QString {
        return newFolderPath + filePath.mid(oldFolderPath.size());
    };
    QVector<QString> newFilePaths = Utils::Fs::renamePaths(renamingFilePaths, nameTransformer, true);
    
    renameFiles(renamingFileIndexes, newFilePaths);
}
