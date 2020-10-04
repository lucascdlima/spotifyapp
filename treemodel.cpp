#include "treemodel.h"
#include "treeitem.h"

#include <QtWidgets>

TreeModel::TreeModel(const QStringList &headers, const QString &data, QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    QVector<QString> rootHeadData;
    for (const QString &header : headers)
    {
        rootData << header;
        rootHeadData<<header;
    }

    rootItem = new TreeItem(rootData,rootHeadData);

    //Lembrar de descomentar
    setupModelData("playlistsdata.json", rootItem);
}


TreeModel::~TreeModel()
{
    delete rootItem;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return rootItem->columnCount();
}


QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);

    return item->data(index.column());
}


Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}


QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}


QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}


bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    beginInsertColumns(parent, position, position + columns - 1);
    const bool success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return false;

    beginInsertRows(parent, position, position + rows - 1);
    const bool success = parentItem->insertChildren(position,
                                                    rows,
                                                    rootItem->columnCount());
    endInsertRows();

    return success;
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(index);
    TreeItem *parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}


bool TreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    beginRemoveColumns(parent, position, position + columns - 1);
    const bool success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return false;

    beginRemoveRows(parent, position, position + rows - 1);
    const bool success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}


int TreeModel::rowCount(const QModelIndex &parent) const
{
    const TreeItem *parentItem = getItem(parent);

    return parentItem ? parentItem->childCount() : 0;
}


bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

    return result;
}

bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    const bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

void TreeModel::setupModelData(const QString path, TreeItem *parent)
{
    QFile loadFile(path);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    const auto root_obj = loadDoc.object();
    if(root_obj.contains("items"))
    {
        const auto playlist_array_json = root_obj.value("items").toArray();

        if(playlist_array_json.size()>0)
        {
            for(int i=0; i<playlist_array_json.size(); i++)
            {
                const auto playlist_obj = playlist_array_json[i].toObject();
                int data_count = playlist_obj.size()-1;
                parent->insertChildren(parent->childCount(), 1, data_count);

                bool headers_ok = playlist_obj.contains("name") && playlist_obj.contains("id") && playlist_obj.contains("uri") && playlist_obj.contains("href") && playlist_obj.contains("items");
                if(headers_ok)
                {
                    parent->child(parent->childCount() - 1)->setData(0,QVariant(playlist_obj.value("name").toString()));
                    parent->child(parent->childCount() - 1)->setData(1,QVariant(playlist_obj.value("id").toString()));
                    parent->child(parent->childCount() - 1)->setData(2,QVariant(playlist_obj.value("href").toString()));
                    parent->child(parent->childCount() - 1)->setData(3,QVariant(playlist_obj.value("uri").toString()));
                    parent->child(parent->childCount() - 1)->setHeadData(0,"name");
                    parent->child(parent->childCount() - 1)->setHeadData(1,"id");
                    parent->child(parent->childCount() - 1)->setHeadData(2,"href");
                    parent->child(parent->childCount() - 1)->setHeadData(3,"uri");


                }

                const auto tracks_array_json = playlist_obj.value("items").toArray();
                TreeItem *playlist_item = parent->child(i);

                for(int j=0; j< tracks_array_json.size(); j++)
                {
                    const auto track_obj = tracks_array_json[j].toObject();
                    int data_count = track_obj.size();


                    playlist_item->insertChildren(playlist_item->childCount(), 1, data_count);

                    bool headers_ok = track_obj.contains("name") && track_obj.contains("id") && track_obj.contains("uri") && track_obj.contains("href");
                    if(headers_ok)
                    {
                        playlist_item->child(playlist_item->childCount() - 1)->setData(0,QVariant(track_obj.value("name").toString()));
                        playlist_item->child(playlist_item->childCount() - 1)->setData(1,QVariant(track_obj.value("id").toString()));
                        playlist_item->child(playlist_item->childCount() - 1)->setData(2,QVariant(track_obj.value("href").toString()));
                        playlist_item->child(playlist_item->childCount() - 1)->setData(3,QVariant(track_obj.value("uri").toString()));
                        playlist_item->child(playlist_item->childCount() - 1)->setHeadData(0,"name");
                        playlist_item->child(playlist_item->childCount() - 1)->setHeadData(1,"id");
                        playlist_item->child(playlist_item->childCount() - 1)->setHeadData(2,"href");
                        playlist_item->child(playlist_item->childCount() - 1)->setHeadData(3,"uri");

                    }
                    else
                        return;

                }

            }
        }
        else
            return;

    }
    else
        return;

                /* QVector<QVariant> parents_data;
            QVector<QVariant> childs_data;
            parents_data.resize(3);
            parents_data[0] = QVariant("timmaia");
            parents_data[1] = QVariant("alceuvalenca");
            parents_data[2] = QVariant("jorgeben");
            childs_data.resize(4);
            childs_data[0] = QVariant("musica1");
            childs_data[1] = QVariant("musica2");
            childs_data[2] = QVariant("musica3");
            childs_data[3] = QVariant("musica4");

            for (int column = 0; column < parents_data.size(); ++column)
            {
                parent->insertChildren(parent->childCount(), 1, 2);
                parent->child(parent->childCount() - 1)->setData(0,QVariant("Nomeplailist"+QString::number(column)));
                parent->child(parent->childCount() - 1)->setData(1,QVariant("IDplaylist"+QString::number(column)));
                index(0,1);


                for(int column2 = 0; column2 < childs_data.size(); ++column2)
                {
                    TreeItem *playlist = parent->child(column);
                    playlist->insertChildren(playlist->childCount(), 1, 3);
                    playlist->child(playlist->childCount() - 1)->setData(0,QVariant("Nometrack"+QString::number(column)));
                    playlist->child(playlist->childCount() - 1)->setData(1,QVariant("IDTrack"+QString::number(column)));
                    playlist->child(playlist->childCount() - 1)->setData(1,QVariant("URITrack"+QString::number(column)));

                }
            }
            */

}
