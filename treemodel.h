#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#define MODEL_TYPE_PLAYLIST 0
#define MODEL_TYPE_TRACK 1


class TreeItem;

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TreeModel(const QStringList &headers, const QString &data,
              QObject *parent = nullptr);
    ~TreeModel();

    int modelType;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headData(const QModelIndex &index) const;
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

    bool saveModelDataOffline(QString filePath);
    void setupModelData(const QString filePath);
    void setupModelFromJson(QJsonObject json_root, int model_type);
    int getModelType();
    bool setupChildrenFromJsonArray(QJsonArray jsonArray, TreeItem *parentItem, QStringList headDataList);

private:
    TreeItem *getItem(const QModelIndex &index) const;
    TreeItem *rootItem;
    QJsonDocument *jsonData;
};


#endif // TREEMODEL_H
