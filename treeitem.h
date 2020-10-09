#ifndef TREEITEM_H
#define TREEITEM_H

#include <QVariant>
#include <QVector>

/**
 * Implementation of TreeItem class based on Qt example to handle data in Tree structure.
 *
 * This class is be used to store the spotify data and represent elements such as Playlists, Tracks and
 * in lower level Artists. Such elements are related in a parent - child relationship.
 * Columns in this class represent data stored of a given TreeItem and rows represent children elements of
 * a TreeItem.
 */
class TreeItem
{
public:
    explicit TreeItem(const QVector<QVariant> &data, const QVector<QString> &headData, TreeItem *parent=nullptr);
    ~TreeItem();
    bool setHeadData(int column, const QString &value);

    //Methods to acess TreeItem information
    TreeItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    QString headData(int column) const;
    TreeItem *parent();
    int childNumber() const;

    //Methods to change tree data and structure
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    bool setData(int column, const QVariant &value);

private:
    QVector<TreeItem*> childItems;
    QVector<QVariant> itemData;
    TreeItem *parentItem;
    QVector<QString> itemHeadData;
};


#endif // TREEITEM_H
