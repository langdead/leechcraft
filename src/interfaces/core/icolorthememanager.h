/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#pragma once

class QColor;

/** @brief Proxy class to the color theme management engine.
 *
 * @sa LeechCraft::Util::ColorThemeProxy.
 */
class Q_DECL_EXPORT IColorThemeManager
{
public:
	virtual ~IColorThemeManager () {}

	/** @brief Returns the color for the given QML section and key.
	 *
	 * See the LeechCraft::Util::ColorThemeProxy for the list of
	 * available color sections and their keys. See the documentation
	 * for that class for the list of possible sections and keys (they
	 * are separated by the \em _ sign).
	 *
	 * @param[in] section The color section like \em ToolButton or
	 * \em Panel.
	 * @param[in] key The key in the \em section like \em TopColor in a
	 * \em ToolButton.
	 * @return The given color for the \em section and \em key, or an
	 * invalid color if the combination is invalid.
	 */
	virtual QColor GetQMLColor (const QString& section, const QString& key) = 0;

	/** @brief Returns the manager as a QObject.
	 *
	 * Use this function to connect to the themeChanged() signal.
	 *
	 * @sa themeChanged()
	 */
	virtual QObject* GetQObject () = 0;
protected:
	/** @brief Emitted after the color theme changes.
	 *
	 * Use the GetQObject() method to get a QObject that emits this signal.
	 *
	 * @sa GetQObject()
	 */
	virtual void themeChanged () = 0;
};

Q_DECLARE_INTERFACE (IColorThemeManager, "org.Deviant.LeechCraft.IColorThemeManager/1.0")
