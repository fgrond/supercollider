/************************************************************************
*
* Copyright 2010 Jakob Leben (jakob.leben@gmail.com)
*
* This file is part of SuperCollider Qt GUI.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
************************************************************************/


#ifndef _SC_QT_COMMON_H
#define _SC_QT_COMMON_H

#include <cstdarg>

namespace QtCollider {
  int debugLevel();
  void setDebugLevel( int );
}

#include <iostream>
#include <QString>

#ifdef QC_DEBUG
  #define qcDebugMsg( LEVEL, MSG ) \
    if( LEVEL <= QtCollider::debugLevel() ) { \
      std::cout << "Qt: " << QString(MSG).toStdString() << "\n"; \
    }
#else
  #define qcDebugMsg( LEVEL, MSG )
#endif

#define qcSCObjectDebugMsg( LEVEL, OBJ, MSG ) \
  qcDebugMsg( LEVEL, QString("[%1] %2") \
                    .arg( OBJ ? slotRawSymbol( &OBJ->classptr->name )->name : "null" ) \
                    .arg(MSG) )

#define qcErrorMsg( MSG ) { std::cout << "Qt: ERROR: " << QString(MSG).toStdString() << "\n"; }

#include <QList>
#include <QVariant>
#include <QEvent>
#include <QMutex>
#include <QWaitCondition>

#include <PyrSymbol.h>
#include <PyrObject.h>

extern pthread_mutex_t gLangMutex;

namespace QtCollider {
  extern PyrSymbol *symSize;
}

struct VariantList {
  QList<QVariant> data;
};

Q_DECLARE_METATYPE( VariantList );


struct QcSyncEvent;

typedef void (*EventHandlerFn) ( QcSyncEvent * );

namespace QtCollider {

  enum MetaType {
    Type_VariantList,
    Type_Count
  };

  enum EventType {
    Event_Sync = QEvent::User,
    Event_ScMethodCall,
    Event_Refresh
  };

  enum Synchronicity {
    Synchronous,
    Asynchronous
  };
}

struct QcSyncEvent : public QEvent
{
  friend class QcApplication;

  enum Type {
    ProxyRequest,
    Generic,
    CreateQObject
  };

  QcSyncEvent( Type type )
    : QEvent( (QEvent::Type) QtCollider::Event_Sync ),
    _cond( 0 ),
    _mutex( 0 ),
    _handler( 0 ),
    _type( type )
  { }

  ~QcSyncEvent()
  {
    if( _cond && _mutex ) {
      _mutex->lock();
      _cond->wakeAll();
      _mutex->unlock();
    }
  }

  Type syncEventType() { return _type; }

  private:

    QWaitCondition *_cond;
    QMutex *_mutex;
    EventHandlerFn _handler;
    Type _type;
};

struct QcGenericEvent : public QcSyncEvent
{
  QcGenericEvent( int type, const QVariant& data = QVariant(), QVariant *ret = 0 )
    : QcSyncEvent( QcSyncEvent::Generic ),
    _data( data ),
    _return( ret ),
    _type( type )
  {}

  template <class T> void returnThis( T arg ) {
    *_return = QVariant::fromValue<T>( arg );
  }

  int genericEventType() { return _type; }

  QVariant _data;
  QVariant *_return;

  private:
    int _type;
};

struct ScMethodCallEvent : public QEvent
{
  ScMethodCallEvent( const QString &s,
                     const QList<QVariant> &l = QList<QVariant>(),
                     bool b_locked = false )
  : QEvent( (QEvent::Type) QtCollider::Event_ScMethodCall ),
    method( getsym( s.toStdString().c_str() ) ),
    args( l ),
    locked( b_locked )
  {}

  ScMethodCallEvent( PyrSymbol *m,
                     const QList<QVariant> &l = QList<QVariant>(),
                     bool b_locked = false )
  : QEvent( (QEvent::Type) QtCollider::Event_ScMethodCall ),
    method( m ),
    args( l ),
    locked( b_locked )
  {}

  PyrSymbol *method;
  QList<QVariant> args;
  bool locked;
};

namespace QtCollider {

  void lockLang();

  inline static void unlockLang()
  {
    pthread_mutex_unlock(&gLangMutex);
    //printf("UNLOCKED\n");
  }

}

#endif //_SC_QT_COMMON_H
