/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2021  Welp Wazowski
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

#pragma once

#include <memory>
#include <QWidget>
#include <QVector>
#include <QString>
#include <QMenu>

#include "base/bittorrent/abstractfilestorage.h"
#include "base/bittorrent/torrentinfo.h"
#include "base/bittorrent/torrent.h"
#include "gui/torrentcontentfiltermodel.h"
#include "gui/properties/proplistdelegate.h"
#include "ui_contentwidget.h"

class ContentWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ContentWidget)

public:
    explicit ContentWidget(QWidget *parent);
    ~ContentWidget() override { };

    // Uses info for display and fileStorage for renaming
    void setTorrent(BitTorrent::TorrentInfo *info);
    void setTorrent(BitTorrent::Torrent *torrent);

    QByteArray saveState() const;
    void loadState(const QByteArray &state);
    // number of bytes among the files selected to be downloaded.
    qlonglong selectedSize() const;
    // if the content widget is backed by a Torrent, update availability and progress
    void loadDynamicData();
    QVector<BitTorrent::DownloadPriority> getFilePriorities() const;
    void clear();

private:
    std::unique_ptr<QMenu> m_actionsMenu;
    std::unique_ptr<PropListDelegate> m_treeViewDelegate;
    std::unique_ptr<Ui::ContentWidget> m_ui;
    std::unique_ptr<TorrentContentFilterModel> m_filterModel;
    BitTorrent::TorrentInfo *m_torrentInfo;
    BitTorrent::Torrent *m_torrent;
    BitTorrent::AbstractFileStorage::RenameList m_undoState;
    bool m_hasUndo;

    // rename the files with the given indexes, prompting the new file name(s)
    void renamePromptMultiple(const QVector<int> &indexes);
    // prompt a new name for the given index into the filter model, and perform the rename.
    void renamePromptSingle(const QModelIndex &index);
    void editPathsPromptMultiple(const QVector<int> &indexes);
    // prompt a new path for the given index into the filter model, and perform the edit.
    void editPathPromptSingle(const QModelIndex &index);
    // return list of torrent indexes, taking only the selected files
    QVector<int> modelIndexesToFileIndexes(const QModelIndexList &) const;
    // Update inner TorrentContentModel after file paths or torrent is changed.
    void setupContentModel();
    // called after a rename completes
    void setupTreeViewAfterRename();
    // call whenever m_torrent or m_torrentInfo are changed.
    void setupChangedTorrent();
    void expandSingleItemFolders();
    void expandSelected();

    void openItem(const QModelIndex &) const;
    void openParentFolder(const QModelIndex &) const;
    void flattenDirectory(const QString &);
    // return the absolute path of the given file/folder. Precondition: using a Torrent (not TorrentInfo)
    QString getFullPath(const QModelIndex &index) const;
    // return all indexes of files
    QModelIndexList allFileIndexes() const;
    // whether the selected files all have the same immediate parent directory
    bool canWrapSelected() const;

    // return whichever out of torrentInfo and torrent is being used as file storage.
    BitTorrent::AbstractFileStorage *fileStorage();
    const BitTorrent::TorrentInfo *torrentInfo() const;
    TorrentContentModel *contentModel();
    QModelIndexList selectedRows() const;

signals:
    // signalled when the set of files selected to be downloaded changes
    void prioritiesChanged();

private slots:
    void renameAll();
    void renameSelected();
    // All renames should go through performEditPaths, because it updates the content model and undo
    // state. It also handles conflicting names and displays a message box.
    void performEditPaths(const BitTorrent::AbstractFileStorage::RenameList &);
    void editPathsAll();
    void editPathsSelected();
    void ensureDirectoryTop();
    void flattenDirectoryTop();
    void flattenDirectoriesAll();
    void wrapSelected(); // wrap the selected files and folders in a new folder
    void relocateSelected();
    void prioritizeInDisplayedOrder(const QModelIndexList &rows);
    void prioritizeInDisplayedOrderAll();
    void prioritizeInDisplayedOrderSelected();
    void undoRename();
    void applyPriorities();
    void doubleClicked();
    void filterText(const QString &);
    void displayTreeMenu(const QPoint &);
    void displayFileListHeaderMenu();
};
