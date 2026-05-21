/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QCryptographicHash>
#include <QDebug>

#include "pageitem_textframe.h"
#include "scribusdoc.h"
#include "scribusview.h"
#include "spellcheckfunctions.h"
#include "text/storytext.h"
#include "textframespellchecker.h"

// Initialize static instance pointer
TextFrameSpellChecker* TextFrameSpellChecker::m_instance = nullptr;

// ============================================================================
// Singleton Management
// ============================================================================

TextFrameSpellChecker* TextFrameSpellChecker::instance()
{
	if (!m_instance)
		m_instance = new TextFrameSpellChecker();
	return m_instance;
}

void TextFrameSpellChecker::destroy()
{
	if (m_instance)
	{
		delete m_instance;
		m_instance = nullptr;
	}
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

TextFrameSpellChecker::TextFrameSpellChecker(QObject* parent)
	: QObject(parent)
{
	// Setup debounce timer
	m_debounceTimer.setSingleShot(true);
	m_debounceTimer.setInterval(m_debounceDelay);
	connect(&m_debounceTimer, &QTimer::timeout, this, &TextFrameSpellChecker::onDebounceTimeout);

	// Create worker thread and worker object
	m_workerThread = new QThread(this);
	m_worker = new SpellCheckerWorker();

	// Move worker to thread
	m_worker->moveToThread(m_workerThread);

	// Connect signals from this object to worker
	connect(this, &TextFrameSpellChecker::requestCheck, m_worker, &SpellCheckerWorker::checkSnapshot);
	
	// Connect results back from worker
	connect(m_worker, &SpellCheckerWorker::checkComplete, this, &TextFrameSpellChecker::onCheckComplete);
	
	// Cleanup when thread finishes
	connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
	
	// Thread lifecycle signals
	connect(m_workerThread, &QThread::started, this, &TextFrameSpellChecker::threadStarted);
	connect(m_workerThread, &QThread::finished, this, &TextFrameSpellChecker::threadStopped);
}

TextFrameSpellChecker::~TextFrameSpellChecker()
{
	stopThread();
	
	// Wait for thread to finish
	if (m_workerThread && m_workerThread->isRunning())
		m_workerThread->wait(5000); // Wait up to 5 seconds
}

// ============================================================================
// Thread Control
// ============================================================================

void TextFrameSpellChecker::startThread()
{
	if (m_workerThread && !m_workerThread->isRunning())
	{
		// qDebug() << "Starting spell checker thread...";
		m_workerThread->start();
	}
}

void TextFrameSpellChecker::stopThread()
{
	if (m_workerThread && m_workerThread->isRunning())
	{
		// qDebug() << "Stopping spell checker thread...";
		
		// Stop the event loop
		m_workerThread->quit();
		
		// Wait for it to finish (with timeout)
		if (!m_workerThread->wait(3000))
		{
			qWarning() << "Spell checker thread did not stop gracefully, forcing termination";
			m_workerThread->terminate();
			m_workerThread->wait();
		}
	}
}

void TextFrameSpellChecker::pauseChecking()
{
	if (m_paused)
		return;
	
	m_paused = true;
	
	// Tell worker to pause
	if (m_worker)
	{
		QMetaObject::invokeMethod(m_worker, [this]() { m_worker->setPaused(true); }, Qt::QueuedConnection);
	}
	
	// Stop debounce timer
	m_debounceTimer.stop();
}

void TextFrameSpellChecker::resumeChecking()
{
	if (!m_paused)
		return;
	
	m_paused = false;
	
	// Tell worker to resume
	if (m_worker)
	{
		QMetaObject::invokeMethod(m_worker, [this]() { m_worker->setPaused(false); }, Qt::QueuedConnection);
	}
	
	// If we have an active frame that needs checking, schedule it
	if (m_activeFrame)
	{
		FrameState& state = getFrameState(m_activeFrame);
		if (state.needsRecheck)
			scheduleCheck(m_activeFrame);
	}
}

bool TextFrameSpellChecker::isThreadRunning() const
{
	return m_workerThread && m_workerThread->isRunning();
}

void TextFrameSpellChecker::setThreadPriority(QThread::Priority priority)
{
	if (m_workerThread)
		m_workerThread->setPriority(priority);
}

// ============================================================================
// Configuration
// ============================================================================

void TextFrameSpellChecker::setEnabled(bool enabled)
{
	// qDebug()<<Q_FUNC_INFO;
	if (m_enabled == enabled)
		return;
	
	m_enabled = enabled;
	
	if (!enabled)
	{
		// Disable: stop debounce timer
		m_debounceTimer.stop();
	}
	else
	{
		// Enable: check active frame if any
		if (m_activeFrame)
			scheduleCheck(m_activeFrame);
	}
}

void TextFrameSpellChecker::setDebounceDelay(int ms)
{
	m_debounceDelay = qMax(0, ms);
	m_debounceTimer.setInterval(m_debounceDelay);
}

void TextFrameSpellChecker::setMaxCachedFrames(int max)
{
	m_maxCachedFrames = qMax(1, max);
	
	// Trim cache if needed
	if (m_frameStates.size() > m_maxCachedFrames)
	{
		while (m_frameStates.size() > m_maxCachedFrames)
		{
			for (auto it = m_frameStates.begin(); it != m_frameStates.end(); ++it)
			{
				if (it.key() != m_activeFrame)
				{
					m_frameStates.erase(it);
					break;
				}
			}
		}
	}
}

// ============================================================================
// Frame Lifecycle Management
// ============================================================================

void TextFrameSpellChecker::frameActivated(PageItem_TextFrame* frame)
{
	// qDebug()<<Q_FUNC_INFO;
	if (!frame || !m_enabled)
		return;
	
	// Make sure thread is running
	if (!isThreadRunning())
	{
		qWarning() << "Spell checker thread not running! Call startThread() first.";
		return;
	}
	
	// If we're switching frames
	if (m_activeFrame != frame)
	{
		m_debounceTimer.stop();
		m_activeFrame = frame;
		m_currentSnapshot = StoryTextSnapshot();
	}

	FrameState& state = getFrameState(frame);

	// Emit cached results if we have them (for immediate display)
	if (!state.cachedErrors.isEmpty() && frame->doc())
	{
		emit resultsReady(frame, state.cachedErrors);
		frame->doc()->regionsChanged()->update(frame->getBoundingRect());
	}

	// Schedule a check if we need one
	if (state.needsRecheck || state.cachedErrors.isEmpty())
		scheduleCheck(frame);
}

void TextFrameSpellChecker::frameDeactivated(PageItem_TextFrame* frame)
{
	if (m_activeFrame != frame)
		return;
	m_activeFrame = nullptr;
	m_debounceTimer.stop();
	m_currentSnapshot = StoryTextSnapshot();
}

void TextFrameSpellChecker::frameTextChanged(PageItem_TextFrame* frame)
{
	// qDebug()<<Q_FUNC_INFO;
	if (!frame || !m_enabled || m_paused)
		return;
	
	// Mark as needing recheck
	FrameState& state = getFrameState(frame);

	QString currentHash = calculateTextHash(frame);
	if (currentHash == state.textHash)
		return;  // Formatting changed but not content — skip recheck
	state.textHash = currentHash;

	state.needsRecheck = true;
	
	// If this is the active frame, schedule a check
	if (frame == m_activeFrame && !m_paused)
		scheduleCheck(frame);
}

void TextFrameSpellChecker::frameLanguageChanged(PageItem_TextFrame* frame)
{
	if (!frame || !m_enabled)
		return;
	
	// Language changed - force immediate recheck
	FrameState& state = getFrameState(frame);
	state.needsRecheck = true;
	state.cachedErrors.clear();
	state.textHash.clear();
	
	if (frame == m_activeFrame && !m_paused)
		checkFrameNow(frame);
}

void TextFrameSpellChecker::frameDeleted(PageItem_TextFrame* frame)
{
	if (!frame)
		return;
	
	if (m_activeFrame == frame)
	{
		m_activeFrame = nullptr;
		m_debounceTimer.stop();
		m_currentSnapshot = StoryTextSnapshot();
	}
	
	if (m_checkingFrame == frame)
		m_checkingFrame = nullptr;
	
	removeFrameState(frame);
}

void TextFrameSpellChecker::documentClosed()
{
	m_debounceTimer.stop();
	m_activeFrame = nullptr;
	m_checkingFrame = nullptr;
	m_currentSnapshot = StoryTextSnapshot();
	m_frameStates.clear();
	++m_generation;
}

void TextFrameSpellChecker::dumpStats() const
{
	qDebug() << "=== SpellChecker Stats ===";
	qDebug() << "  Cached frames:" << m_frameStates.size();
	qDebug() << "  Generation:" << m_generation;
	qDebug() << "  Active frame:" << m_activeFrame;
	qDebug() << "  Checking frame:" << m_checkingFrame;
	qDebug() << "  Checks performed:" << m_checkCount;

	int totalErrors = 0;
	int totalStringBytes = 0;
	for (auto it = m_frameStates.constBegin(); it != m_frameStates.constEnd(); ++it)
	{
		totalErrors += it->cachedErrors.size();
		for (const SpellError& e : it->cachedErrors)
			totalStringBytes += (e.word.size() + e.language.size()) * sizeof(QChar);
		totalStringBytes += it->textHash.size() * sizeof(QChar);
	}

	qDebug() << "  Total cached errors:" << totalErrors;
	qDebug() << "  Approx string memory:" << totalStringBytes << "bytes";
	qDebug() << "========================";
}

// ============================================================================
// Checking
// ============================================================================

void TextFrameSpellChecker::checkFrameNow(PageItem_TextFrame* frame)
{
	if (!frame || !m_enabled || m_paused)
		return;
	
	if (!isThreadRunning())
	{
		qWarning() << "Spell checker thread not running!";
		return;
	}
	
	m_debounceTimer.stop();
	performCheck(frame);
}

void TextFrameSpellChecker::scheduleCheck(PageItem_TextFrame* frame)
{
	if (!frame || !m_enabled || m_paused)
		return;
	
	if (frame != m_activeFrame)
		return;
	
	m_debounceTimer.start();
}

void TextFrameSpellChecker::onDebounceTimeout()
{
	if (m_activeFrame && m_enabled)
		performCheck(m_activeFrame);
}

void TextFrameSpellChecker::performCheck(PageItem_TextFrame* frame)
{
	// qDebug()<<Q_FUNC_INFO;
	if (!frame || !m_enabled || m_paused)
		return;

	if (!isThreadRunning())
	{
		qWarning() << "Spell checker thread not running!";
		return;
	}

	// Don't queue another snapshot if one is already pending for this frame
	if (m_checkingFrame == frame)
	{
		// Mark it so we recheck after the current one completes
		FrameState& state = getFrameState(frame);
		state.needsRecheck = true;
		return;
	}

	++m_checkCount;
	
	// Create snapshot
	m_currentSnapshot = frame->itemText.createSnapshot();
	
	// Calculate and store text hash
	FrameState& state = getFrameState(frame);
	state.textHash = calculateTextHash(frame);
	state.needsRecheck = false;
	
	// Remember which frame we're checking
	m_checkingFrame = frame;
	
	// Emit signal
	emit checkStarted(frame);
	
	// Send to worker thread via signal
	// The snapshot is copied (safely) when emitted
	emit requestCheck(frame, m_currentSnapshot, m_generation);
	
	// Clear snapshot in main thread (worker has its own copy)
	m_currentSnapshot = StoryTextSnapshot();
}

void TextFrameSpellChecker::onCheckComplete(PageItem_TextFrame* frame, const QVector<SpellError>& errors, int generation)
{
	// This slot runs in the MAIN thread (connected via Qt::AutoConnection)

	// Discard results from a previous document
	if (generation != m_generation)
		return;

	if (!frame)
		return;
	
	// Store in cache
	FrameState& state = getFrameState(frame);

	// Don't trigger a repaint if nothing changed
	if (state.cachedErrors.size() == errors.size() && state.cachedErrors == errors)
	{
		if (m_checkingFrame == frame)
			m_checkingFrame = nullptr;
		// Still check if a recheck was requested while we were busy
		if (state.needsRecheck && frame == m_activeFrame)
			performCheck(frame);
		return;
	}

	state.cachedErrors = errors;

	// Emit results
	emit resultsReady(frame, errors);
	
	// Clear checking frame if it matches
	if (m_checkingFrame == frame)
		m_checkingFrame = nullptr;

	// Just repaint the frame area — no layout invalidation needed
	if (frame->doc())
		frame->doc()->regionsChanged()->update(frame->getBoundingRect());

	// If text changed again while we were checking, go again
	if (state.needsRecheck && frame == m_activeFrame)
		performCheck(frame);
}

// ============================================================================
// Query Methods
// ============================================================================

QVector<SpellError> TextFrameSpellChecker::getErrors(PageItem_TextFrame* frame)
{
	auto it = m_frameStates.constFind(frame);
	if (it == m_frameStates.end())
		return QVector<SpellError>();
	return it->cachedErrors;
}

bool TextFrameSpellChecker::hasResults(PageItem_TextFrame* frame) const
{
	auto it = m_frameStates.find(frame);
	return (it != m_frameStates.end() && !it->cachedErrors.isEmpty());
}

bool TextFrameSpellChecker::isCheckingFrame(PageItem_TextFrame* frame) const
{
	return (m_checkingFrame == frame);
}

void TextFrameSpellChecker::cancelAll()
{
	m_debounceTimer.stop();
	m_checkingFrame = nullptr;
	m_currentSnapshot = StoryTextSnapshot();
	
	// Note: We can't cancel work already sent to the worker thread
	// (it doesn't have a cancel mechanism built in yet)
	// But we can ignore results by clearing m_checkingFrame
}

void TextFrameSpellChecker::clearCache()
{
	m_frameStates.clear();
}

TextFrameSpellChecker::FrameState& TextFrameSpellChecker::getFrameState(PageItem_TextFrame* frame)
{
	if (!m_frameStates.contains(frame) && m_frameStates.size() >= m_maxCachedFrames)
	{
		for (auto it = m_frameStates.begin(); it != m_frameStates.end(); ++it)
		{
			if (it.key() != m_activeFrame && it.key() != m_checkingFrame)
			{
				m_frameStates.erase(it);
				break;
			}
		}
	}
	
	return m_frameStates[frame];
}

void TextFrameSpellChecker::removeFrameState(PageItem_TextFrame* frame)
{
	m_frameStates.remove(frame);
}

QString TextFrameSpellChecker::calculateTextHash(PageItem_TextFrame* frame) const
{
	if (!frame)
		return QString();
	
	QString text = frame->itemText.plainText();
	
	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(text.toUtf8());
	
	return QString::fromLatin1(hash.result().toHex());
}

// ============================================================================
// Worker Implementation
// ============================================================================

SpellCheckerWorker::SpellCheckerWorker(QObject* parent)
	: QObject(parent)
{
}

SpellCheckerWorker::~SpellCheckerWorker()
{
	// qDebug() << "SpellCheckerWorker destroyed";
}

void SpellCheckerWorker::setPaused(bool paused)
{
	m_paused = paused;
}

void SpellCheckerWorker::checkSnapshot(PageItem_TextFrame* frame, const StoryTextSnapshot& snapshot, int generation)
{
	// This runs in the WORKER thread
	
	if (m_paused)
	{
		qDebug() << "Worker is paused, ignoring check request";
		return;
	}

	// Perform spell check
	QVector<SpellError> errors = performSpellCheck(snapshot);
	// qDebug()<<"Perform spell check";

	// Map positions back to original StoryText coordinates
	for (int i = 0; i < errors.size(); ++i)
	{
		int origStart, origLength;
		if (snapshot.mapRangeToOriginal(errors[i].position, errors[i].length, origStart, origLength))
		{
			errors[i].position = origStart;
			errors[i].length = origLength;
		}
	}

	// Emit results back to main thread
	emit checkComplete(frame, errors, generation);
}

