#include "treemodel.h"
#include "treeitem.h"

#include <QtWidgets>

TreeModel::TreeModel(const QStringList &headers, QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    QVector<QString> rootHeadData;
    for (const QString &header : headers)
    {
        rootData << header;
        rootHeadData<<header;
    }

    //Sets head labels and data(empty) for root item in the tree head labels are the
    //ones shown in QTreeView heads.
    rootItem = new TreeItem(rootData,rootHeadData);
}


TreeModel::~TreeModel()
{
    delete rootItem;
}

/**
*Method to get the number of column data in the Treeitem object given by its QMidelIndex. All TreeItem
*objects have the same number of data columns in this tree model.
*@param parent index correspondent to TreeItem.
*@return the number of columns.
*/
int TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return rootItem->columnCount();
}

/**
*Method to get the data of a TreeItem using a QModelIndex parameter. Each column of data in the TreeItem
*is represented by a QModelIntex object. During creation of QModelIndex indexes they are internally related to
*TreeItem objects so that the TreeModel is able to access them later on in the code.
*@param index correspondent to TreeItem data column.
*@param role the Qt::DisplayRole of the data
*@return the data in QVariant, if a non existent index is passed returns Null object
*/
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);

    return item->data(index.column());
}

/**
*Method to get the head data (labels) of data columns in TreeItem objects. Same operations as
*the method data() presented previously.
*@param index correspondent to TreeItem data column.
*@return the head label data. If a non existent index is passed, returns a Null object.
*/
QString TreeModel::headData(const QModelIndex &index) const
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

/**
*Method to get the head data (labels) of data columns in TreeItem objects. Same operations as
*the method data() presented previously.
*@param index correspondent to TreeItem data column.
*@return the head data in QString, if a non existent index is passed returns Null object.
*/
TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

/**
*Overloaded method QAbstractModelItem::headerData(int section, Qt::Orientation orientation, int role)
*Get the header data of the root TreeItem.
*/
QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

/**
*Method to get the index correspondent to a data column and TreeItem child row given the
*parent index. Tgis method allows to get data intdex for all TreeItems, parents and childs in
*the Tree structure.
*@param row number of child row position in the TreeItem parent structure.
*@param column number of column data position in the data array
*@param parent the index of parent TreeItem object.
*@return QModelIndex of the respective data in row and column positions, if the row or column don't exist or the parent
* is invalid, return a Null object.
*/
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    //QModel indexes representing any column different from zero in TreeItem is not considered parent
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

bool TreeModel::setHeadData(const QModelIndex &index, const QString &value)
{
    TreeItem *item = getItem(index);
    return item->setHeadData(index.column(), value);
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

/**
*Method to create a Tree structure with parents and children TreeItem through a external saved data.
*For this application the file data must be in (.json) format.
*@param filePath the file name and path.
*/
bool TreeModel::loadModelData(const QString filePath)
{
    modelType = MODEL_TYPE_PLAYLIST;
    QFile loadFile(filePath);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return 0;
    }

    QByteArray saveData = loadFile.readAll();

    QStringList childrenHeader = {"name","id","href","uri"};

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    const auto root_obj = loadDoc.object();

    QStringList arrayLevels = {"playlists", "tracks", "artists"};

    //Insert recursively children in the rootItem
    if(!AddChildrenFromJson(root_obj,rootItem,arrayLevels,childrenHeader))
    {
        qDebug()<<"Model not set"<<endl;
        return 0;
    }

    return 1;

}

/**
*Method to create a Tree structure with parents and children TreeItem through a external saved data.
*For this application the file data must be in (.json) format.
*@param filePath the file name and path.
*/
bool TreeModel::loadModelData(QJsonObject parentJson , int model_type)
{
    QStringList arrayLevels = {"playlists","tracks","artists"};
    if(model_type == MODEL_TYPE_TRACK)
        arrayLevels.pop_front();

    QStringList childrenHeader = {"name","id","href","uri"};

    //Insert recursively children in the rootItem
    if(!AddChildrenFromJson(parentJson,rootItem,arrayLevels,childrenHeader))
    {
        qDebug()<<"Model not set "<<endl;
        return 0;
    }

    return 1;
}

/**
*Method recursive that add children to parent TreeItem and set respective data to the items models.
*The param itemArrays determine if a parent ItemTree has children or not.
*@param parentJson object with data of children itens data.
*@param parentItem parent object that will receive TreeItem children objects.
*@param itemsArrays list with the labels of children in the QJsonObject parentJson objects. In each recursion this list
*is reduced by one element.
*@param headers List with the head labels of TreeItem's data.
*/
bool TreeModel::AddChildrenFromJson(QJsonObject parentJson, TreeItem *parentItem, QStringList itemsArrays, QStringList headers)
{
    if(itemsArrays.size()==0)
        return 0;

    QString childrenLabel = itemsArrays[0];
    itemsArrays.pop_front();

    if(!parentJson.contains(childrenLabel))
        return 0;

    const auto childrenArrayJson = parentJson.value(childrenLabel).toArray();
    if(childrenArrayJson.size()<=0)
        return 0;

    if(headers.size()==0)
        return 0;

    int dataCount = headers.size();

    for(int i=0; i<childrenArrayJson.size(); i++)
    {
        const auto childItemJson = childrenArrayJson[i].toObject();

        //Child item is created in parentItem item tree
        parentItem->insertChildren(parentItem->childCount(), 1, rootItem->columnCount());

        //loop to set child data
        for (int j=0; j< dataCount;j++)
        {
            if(!childItemJson.contains(headers[j]))
            {
                qDebug()<<"Model child not created: Array object data incomplete"<<endl;
                parentItem->removeChildren(0,parentItem->childCount());
                return 0;
            }
            parentItem->child(parentItem->childCount() - 1)->setData(j,QVariant(childItemJson.value(headers[j]).toString()));
            parentItem->child(parentItem->childCount() - 1)->setHeadData(j,headers[j]);
        }

        //Adds artist information to tracks column data for display purposes
        if(parentJson.contains("artists"))
        {
            const auto childItemJson0 = childrenArrayJson[0].toObject();
            parentItem->setData(parentItem->columnCount()-1,childItemJson0.value("name").toString());
            parentItem->setHeadData(parentItem->columnCount()-1,"artist");
        }

        if(itemsArrays.size()==0)
            return 1;
        else
        {   //Call the method again to insert children in the current inserted TreeItem objects
            if(!AddChildrenFromJson(childItemJson,parentItem->child(parentItem->childCount() - 1),itemsArrays,headers))
            {
                parentItem->removeChildren(0,parentItem->childCount());
                return 0;
            }
        }
    }
    return 1;
}

/**
*Method to save the current user playlists Tree model in (.json). It crates a Json root object and adds information
*from the TreeItem objects of the model.
*@param filePath object with data of children itens data.
*@return True if the model is saved correctly, false otherwise.
*/
bool TreeModel::saveModelDataOffline(QString filePath)
{

    QJsonDocument playlistsJsonDoc;
    QJsonObject root_obj;
    root_obj.insert("info", QJsonValue::fromVariant("Document of Spotify playlists offline in JSOn format"));
    QJsonArray playlist_array;

    QStringList childrenHeader = {"name","id","href","uri"};

    int N_playlists = rowCount();
    for(int i=0; i < N_playlists;i++)
    {
        QJsonObject playlistObj;
        //Get each playlist index to retrieve data
        auto playlists_item_index = index(i,0);
        if(hasIndex(i,0))
        {
            QModelIndex data_index;
            for (int k=0;k< columnCount(playlists_item_index); k++)
            {
                //Get playlists data indexes and adds respective data to json object
                auto data_index = index(i,k);
                //Insert key-data pairs in playlist Json object
                if(childrenHeader.contains(headData(data_index)))
                    playlistObj.insert(headData(data_index),data_index.data().toString());
            }
        }
        else
        {
            qDebug()<<"Error - Playlists not saved: Problem with child index"<<endl;
            return false;
        }
        QJsonArray tracksArray;
        int N_tracks = rowCount(playlists_item_index);
        for(int j=0;j<N_tracks; j++)
        {
            QJsonObject trackObj;
            QModelIndex track_item_index;
            if(hasIndex(j,0,playlists_item_index))
            {
                //Get each track child index in the current playlist item
                track_item_index = index(j,0,playlists_item_index);

                QModelIndex data_index;
                for (int k=0;k< columnCount(track_item_index); k++)
                {
                    auto data_index = index(j,k,playlists_item_index);
                    //Insert key-data pairs in track Json object
                    if(childrenHeader.contains(headData(data_index)))
                        trackObj.insert(headData(data_index),data_index.data().toString());
                }
            }
            else
            {
                qDebug()<<"Error - Playlists not saved: Problem with child index"<<endl;
                return false;
            }

            QJsonArray artistsArray;
            int N_artists= rowCount(track_item_index);
            for(int r=0;r<N_artists; r++)
            {
                QModelIndex artist_item_index;
                if(hasIndex(r,0,track_item_index))
                {
                    QJsonObject artistObj;

                    //get child index of each artist item data in the current track item
                    artist_item_index = index(r,0,track_item_index);

                    QModelIndex data_index;
                    for (int k=0;k< columnCount(artist_item_index); k++)
                    {
                        auto data_index = index(r,k,track_item_index);
                        //Insert key-data pairs in artist Json object
                        if(childrenHeader.contains(headData(data_index)))
                            artistObj.insert(headData(data_index),data_index.data().toString());
                    }
                    artistsArray.append(artistObj);
                }
                else
                {
                    qDebug()<<"Error - Playlists not saved: Problem with child index"<<endl;
                    return false;
                }
            }
            trackObj.insert("artists",artistsArray);
            tracksArray.append(trackObj);

        }
        playlistObj.insert("tracks",tracksArray);
        playlist_array.append(playlistObj);

    }

    root_obj.insert("playlists",playlist_array);

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

/**
*Method to get a column data of a TreeItem object with the given head label.
*@param headName head label that identify the column data in search.
*@param itemIndex index of the TreeItem object where data is stored.
*@return returns the data in QVariant. If data is not found or the index argument is invalid, returns a Null object.
*/
QVariant TreeModel::findDataByHead(QString headName, QModelIndex &itemIndex)
{
    if(!itemIndex.isValid())
        return QVariant();

    TreeItem *item = getItem(itemIndex);
    if(item==rootItem)
        return QVariant();

    for( int i=0; i< item->columnCount(); i++)
        if(item->headData(i)==headName)
            return item->data(i);

    return QVariant();

}

int TreeModel::getModelType()
{
    return modelType;
}
