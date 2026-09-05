/****************************************************************************
** Meta object code from reading C++ file 'scrcpy_widget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "scrcpy_widget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scrcpy_widget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ScrcpyWidget_t {
    QByteArrayData data[12];
    char stringdata0[128];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScrcpyWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScrcpyWidget_t qt_meta_stringdata_ScrcpyWidget = {
    {
QT_MOC_LITERAL(0, 0, 12), // "ScrcpyWidget"
QT_MOC_LITERAL(1, 13, 9), // "connected"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 5), // "width"
QT_MOC_LITERAL(4, 30, 6), // "height"
QT_MOC_LITERAL(5, 37, 12), // "disconnected"
QT_MOC_LITERAL(6, 50, 13), // "errorOccurred"
QT_MOC_LITERAL(7, 64, 3), // "msg"
QT_MOC_LITERAL(8, 68, 15), // "toggleRequested"
QT_MOC_LITERAL(9, 84, 17), // "resolutionChanged"
QT_MOC_LITERAL(10, 102, 15), // "maximizeToggled"
QT_MOC_LITERAL(11, 118, 9) // "maximized"

    },
    "ScrcpyWidget\0connected\0\0width\0height\0"
    "disconnected\0errorOccurred\0msg\0"
    "toggleRequested\0resolutionChanged\0"
    "maximizeToggled\0maximized"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScrcpyWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   44,    2, 0x06 /* Public */,
       5,    0,   49,    2, 0x06 /* Public */,
       6,    1,   50,    2, 0x06 /* Public */,
       8,    0,   53,    2, 0x06 /* Public */,
       9,    2,   54,    2, 0x06 /* Public */,
      10,    1,   59,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    3,    4,
    QMetaType::Void, QMetaType::Bool,   11,

       0        // eod
};

void ScrcpyWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScrcpyWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connected((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->disconnected(); break;
        case 2: _t->errorOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->toggleRequested(); break;
        case 4: _t->resolutionChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->maximizeToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScrcpyWidget::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScrcpyWidget::connected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScrcpyWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScrcpyWidget::disconnected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ScrcpyWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScrcpyWidget::errorOccurred)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ScrcpyWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScrcpyWidget::toggleRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ScrcpyWidget::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScrcpyWidget::resolutionChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ScrcpyWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScrcpyWidget::maximizeToggled)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ScrcpyWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ScrcpyWidget.data,
    qt_meta_data_ScrcpyWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ScrcpyWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScrcpyWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScrcpyWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ScrcpyWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ScrcpyWidget::connected(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScrcpyWidget::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ScrcpyWidget::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ScrcpyWidget::toggleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ScrcpyWidget::resolutionChanged(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ScrcpyWidget::maximizeToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
