#ifndef TREEITEM_H
#define TREEITEM_H

#include <QVariant>
#include <QVector>

//! [0]
class TreeItem
{
public:
    explicit TreeItem(const QVector<QVariant> &data, const QVector<QString> &headData, TreeItem *parent=nullptr);
    ~TreeItem();
    bool setHeadData(int column, const QString &value);

    TreeItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    QString headData(int column) const;
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    TreeItem *parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant &value);

private:
    QVector<TreeItem*> childItems;
    QVector<QVariant> itemData;
    TreeItem *parentItem;
    QVector<QString> itemHeadData;
};
//! [0]

#endif // TREEITEM_H
