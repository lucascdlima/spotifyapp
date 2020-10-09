#include "treeitem.h"


TreeItem::TreeItem(const QVector<QVariant> &data, const QVector<QString> &headData, TreeItem *parent)
    : itemData(data),
      itemHeadData(headData),
      parentItem(parent)
{}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

TreeItem *TreeItem::child(int number)
{
    if (number < 0 || number >= childItems.size())
        return nullptr;
    return childItems.at(number);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

/**
Method to get the row number of the object in its parent tree. If a parent item is null
than the current item is the root of the tree.
@return the row number of this object.
*/
int TreeItem::childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    return 0;
}

int TreeItem::columnCount() const
{
    return itemData.count();
}

QVariant TreeItem::data(int column) const
{
    if (column < 0 || column >= itemData.size())
        return QVariant();
    return itemData.at(column);
}

/**
Method to get the head(label) QString of each data stored in the ItemTree.
@param column number of column data.
@return the head label of data.
*/
QString TreeItem::headData(int column) const
{
    if (column < 0 || column >= itemHeadData.size())
        return QString();
    return itemHeadData.at(column);
}

/**
Method to insert ItemTree child objects in the current level.
@param position position in the current tree where the children rows (TreeItem objects) will be inserted.
@param count number of child TreeItem objects that will be inserted.
@param columns number of columns data of each TreeItem child inserted.
@return true if children are inserted.
*/
bool TreeItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        QVector<QString> head_data(columns);
        TreeItem *item = new TreeItem(data, head_data, this);
        childItems.insert(position, item);
    }

    return true;
}

/**
Method to insert data columns in the current TreeItem. The current parent and all children items
will have columns isnerted.
@param position position in the current tree where the data columns will be inserted.
@param columns number of columns data that will be inserted.
@return the head label of the data column.
*/
bool TreeItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
    {
        itemData.insert(position, QVariant());
        itemHeadData.insert(position,QString());
    }

    for (TreeItem *child : qAsConst(childItems))
            child->insertColumns(position, columns);

    return true;
}

TreeItem *TreeItem::parent()
{
    return parentItem;
}

bool TreeItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

bool TreeItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
    {
        itemData.remove(position);
        itemHeadData.remove(position);
    }

    for (TreeItem *child : qAsConst(childItems))
            child->removeColumns(position, columns);

    return true;
}

bool TreeItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

bool TreeItem::setHeadData(int column, const QString &value)
{
    if (column < 0 || column >= itemHeadData.size())
        return false;

    itemHeadData[column] = value;
    return true;
}
