#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#define MODEL_TYPE_PLAYLIST 0
#define MODEL_TYPE_TRACK 1

/**
 * Implementation of TreeModel class based on Qt example to handle TreeItem objects and allow the Qt interface
 * to display data of TreeItem objects using Qt view classes (QTreeView, QTableView, QListView, etc.)
 *
 * This class inherits the QAbstractItemModel implementing basic methods to allow data management and
 * structure modifications in TreeItem objects.
 * The data in TreeItem objects are acessed through QModelIndex indexes that are set inside the TreeModel class.
 * It is through the referred QModelIndex indexes that interfaces (Qt views) access items data for displaying
 * purposes.
 */
class TreeItem;

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TreeModel(const QStringList &headers, QObject *parent = nullptr);
    ~TreeModel();

    int modelType;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole) override;
    bool insertColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

    QString headData(const QModelIndex &index) const;
    bool setHeadData(const QModelIndex &index, const QString &value);
    QVariant findDataByHead(QString headName, QModelIndex &parent);

    bool saveModelDataOffline(QString filePath);
    bool loadModelData(const QString filePath);
    bool loadModelData(QJsonObject parentJson, int model_type);
    bool AddChildrenFromJson(QJsonObject parentJson, TreeItem *parentItem, QStringList itemsArrays, QStringList headers);
    int getModelType();



private:
    TreeItem *getItem(const QModelIndex &index) const;
    TreeItem *rootItem;
    QJsonDocument *jsonData;
};


#endif // TREEMODEL_H
