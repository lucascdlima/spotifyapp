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

QVariant TreeModel::headData(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();

    TreeItem *item = getItem(index);

    return item->headData(index.column());
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

void TreeModel::setupModelData(const QString filePath)
{
    modelType = MODEL_TYPE_PLAYLIST;
    QFile loadFile(filePath);

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
                rootItem->insertChildren(rootItem->childCount(), 1, data_count);

                bool headers_ok = playlist_obj.contains("name") && playlist_obj.contains("id") && playlist_obj.contains("uri") && playlist_obj.contains("href") && playlist_obj.contains("items");
                if(headers_ok)
                {
                    rootItem->child(rootItem->childCount() - 1)->setData(0,QVariant(playlist_obj.value("name").toString()));
                    rootItem->child(rootItem->childCount() - 1)->setData(1,QVariant(playlist_obj.value("id").toString()));
                    rootItem->child(rootItem->childCount() - 1)->setData(2,QVariant(playlist_obj.value("href").toString()));
                    rootItem->child(rootItem->childCount() - 1)->setData(3,QVariant(playlist_obj.value("uri").toString()));
                    rootItem->child(rootItem->childCount() - 1)->setHeadData(0,"name");
                    rootItem->child(rootItem->childCount() - 1)->setHeadData(1,"id");
                    rootItem->child(rootItem->childCount() - 1)->setHeadData(2,"href");
                    rootItem->child(rootItem->childCount() - 1)->setHeadData(3,"uri");


                }

                const auto tracks_array_json = playlist_obj.value("items").toArray();
                TreeItem *playlist_item = rootItem->child(i);

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

}

bool TreeModel::saveModelDataOffline(QString filePath)
{
    QJsonDocument playlistsJsonDoc;
    QJsonObject root_obj;
    root_obj.insert("info", QJsonValue::fromVariant("Document of Spotify playlists offline in JSOn format"));
    QJsonArray playlist_array;

    int N_playlists = rowCount();
    for(int i=0; i < N_playlists;i++)
    {
        QJsonObject playlist_obj;
        auto playlists_item_index = index(i,0);
        if(hasIndex(i,0))
        {

            auto data_index = index(i,0);
            playlist_obj.insert("name",data_index.data().toString());
            data_index = index(i,1);
            playlist_obj.insert("id",data_index.data().toString());
            data_index = index(i,2);
            playlist_obj.insert("href",data_index.data().toString());
            data_index = index(i,3);
            playlist_obj.insert("uri",data_index.data().toString());

        }
        else
        {
            qDebug()<<"Error - Playlists not saved: Problem with child index"<<endl;
            return false;
        }
        QJsonArray tracks_array;
        int N_tracks = rowCount(playlists_item_index);
        for(int k=0;k<N_tracks; k++)
        {
            if(hasIndex(k,0,playlists_item_index))
            {
                QJsonObject track_obj;

                auto data_index = index(k,0,playlists_item_index);
                track_obj.insert("name",data_index.data().toString());
                data_index = index(k,1,playlists_item_index);
                track_obj.insert("id",data_index.data().toString());
                data_index = index(k,2,playlists_item_index);
                track_obj.insert("href",data_index.data().toString());
                data_index = index(k,3,playlists_item_index);
                track_obj.insert("uri",data_index.data().toString());
                tracks_array.append(track_obj);

            }
            else
            {
                qDebug()<<"Error - Playlists not saved: Problem with child index"<<endl;
                return false;
            }

        }
        playlist_obj.insert("items",tracks_array);
        playlist_array.append(playlist_obj);

    }

    root_obj.insert("items",playlist_array);


    playlistsJsonDoc.setObject(root_obj);

    QFile saveFile(filePath);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    if(saveFile.write(playlistsJsonDoc.toJson())==-1)
    {
        qDebug()<<"Error during write operation to file"<<endl;
        return false;

    }

    return true;

}

void TreeModel::setupModelFromJson(QJsonObject json_root,int model_type)
{
    modelType = model_type;
    if(json_root.isEmpty())
    {
        qDebug()<<"Model not created: Json object is empty"<<endl;
        return;
    }

    if(!json_root.contains("tracks"))
    {
        qDebug()<<"Model not created: Json object doesn't have tracks data"<<endl;
        return;
    }
    const auto tracksArray = json_root.find("tracks")->toArray();
    if(tracksArray.size()==0)
    {
        qDebug()<<"Model not created: Tracks not found"<<endl;
        return;
    }
    int data_count = tracksArray[0].toObject().size()-1;

    for (int i=0; i< tracksArray.size();i++)
    {
        rootItem->insertChildren(rootItem->childCount(), 1, data_count);
        const auto trackObj = tracksArray[i].toObject();
        rootItem->child(rootItem->childCount() - 1)->setData(0,QVariant(trackObj.value("name").toString()));
        rootItem->child(rootItem->childCount() - 1)->setData(1,QVariant(trackObj.value("id").toString()));
        rootItem->child(rootItem->childCount() - 1)->setData(2,QVariant(trackObj.value("href").toString()));
        rootItem->child(rootItem->childCount() - 1)->setData(3,QVariant(trackObj.value("uri").toString()));
        rootItem->child(rootItem->childCount() - 1)->setHeadData(0,"name");
        rootItem->child(rootItem->childCount() - 1)->setHeadData(1,"id");
        rootItem->child(rootItem->childCount() - 1)->setHeadData(2,"href");
        rootItem->child(rootItem->childCount() - 1)->setHeadData(3,"uri");
        if(!trackObj.contains("artists"))
        {
            rootItem->removeChildren(0,rootItem->childCount());
            qDebug()<<"Model not created: Track artist not found"<<endl;
            return;
        }
        const auto artistsArray = trackObj.value("artists").toArray();

        if(artistsArray.size()==0)
        {
            rootItem->removeChildren(0,rootItem->childCount());
            qDebug()<<"Model not created: Track artist not found"<<endl;
            return;
        }

        TreeItem *trackItem = rootItem->child(i);
        QStringList childrenHeader = {"name","id","href","uri"};
        if(!setupChildrenFromJsonArray(artistsArray,trackItem,childrenHeader))
        {
            rootItem->removeChildren(0,rootItem->childCount());
            return;
        }


        /*for (int j=0; j< artistsArray.size();j++)
        {
            const auto artistObj = artistsArray[j].toObject();

            int artistDataCount = artistObj.size();

            trackItem->insertChildren(trackItem->childCount(), 1, artistDataCount);

           if(!(artistObj.contains("name") && artistObj.contains("id") && artistObj.contains("uri") && artistObj.contains("href")))
            {
                rootItem->removeChildren(0,rootItem->childCount());
                qDebug()<<"Model not created: Artist data incomplete not found"<<endl;
                return;
            }
            trackItem->child(trackItem->childCount() - 1)->setData(0,QVariant(artistObj.value("name").toString()));
            trackItem->child(trackItem->childCount() - 1)->setData(1,QVariant(artistObj.value("id").toString()));
            trackItem->child(trackItem->childCount() - 1)->setData(2,QVariant(artistObj.value("href").toString()));
            trackItem->child(trackItem->childCount() - 1)->setData(3,QVariant(artistObj.value("uri").toString()));
            trackItem->child(trackItem->childCount() - 1)->setHeadData(0,"name");
            trackItem->child(trackItem->childCount() - 1)->setHeadData(1,"id");
            trackItem->child(trackItem->childCount() - 1)->setHeadData(2,"href");
            trackItem->child(trackItem->childCount() - 1)->setHeadData(3,"uri");

        }*/

    }

    qDebug()<<"Model created correctly"<<endl;


}

bool TreeModel::setupChildrenFromJsonArray(QJsonArray jsonArray, TreeItem *parentItem, QStringList headDataList)
{
    if(jsonArray.size()==0)
    {
        qDebug()<<"Model child not created: Json Array is empty"<<endl;
        return 0;
    }
    for (int i=0; i< jsonArray.size();i++)
    {
        const auto childObj = jsonArray[i].toObject();

        int dataCount = childObj.size();
        if(headDataList.size()!=dataCount)
        {
            qDebug()<<"QJson array objects data and head data don't have same size"<<endl;
            return 0;
        }
        parentItem->insertChildren(parentItem->childCount(), 1, dataCount);

        for (int j=0; j< dataCount;j++)
        {
            if(!childObj.contains(headDataList[j]))
            {
                rootItem->removeChildren(0,rootItem->childCount());
                qDebug()<<"Model child not created: Array object data incomplete"<<endl;
                return 0;
            }
            parentItem->child(parentItem->childCount() - 1)->setData(j,QVariant(childObj.value(headDataList[j]).toString()));
            parentItem->child(parentItem->childCount() - 1)->setHeadData(j,headDataList[j]);
        }
    }
    return 1;

}

int TreeModel::getModelType()
{
    return modelType;
}
