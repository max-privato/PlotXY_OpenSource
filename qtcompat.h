/*
 * qtcompat.h  — Qt 5 / Qt 6 compatibility helpers for FunPlot
 *
 * Include this header wherever Qt-version-specific APIs are used.
 */

#ifndef QTCOMPAT_H
#define QTCOMPAT_H

#include <QRegularExpression>
#include <QString>

// ---------------------------------------------------------------------------
// regExpIndexIn()
//   Mimics QRegExp::indexIn(str, from) using QRegularExpression.
//   - Returns the start position of the first match at or after 'from', or -1.
//   - Negative 'from' counts from the end of the string
//     (e.g. from = -1  →  last character), matching QRegExp behaviour.
// ---------------------------------------------------------------------------
inline int regExpIndexIn(const QRegularExpression &re,
                         const QString &str,
                         int from = 0)
{
    int actualFrom = (from < 0) ? qMax(0, str.length() + from) : from;
    QRegularExpressionMatch m = re.match(str, actualFrom);
    return m.hasMatch() ? m.capturedStart() : -1;
}

// ---------------------------------------------------------------------------
// Mouse-event position helpers
//   Qt 5: QMouseEvent::pos(), globalPos(), x(), y()  (no deprecation)
//   Qt 6: the above are deprecated; use position() / globalPosition() instead
// ---------------------------------------------------------------------------
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define QME_POS(e)        (e)->position().toPoint()
#  define QME_GLOBAL_POS(e) (e)->globalPosition().toPoint()
#  define QME_X(e)          static_cast<int>((e)->position().x())
#  define QME_Y(e)          static_cast<int>((e)->position().y())
#else
#  define QME_POS(e)        (e)->pos()
#  define QME_GLOBAL_POS(e) (e)->globalPos()
#  define QME_X(e)          (e)->x()
#  define QME_Y(e)          (e)->y()
#endif

#endif // QTCOMPAT_H
