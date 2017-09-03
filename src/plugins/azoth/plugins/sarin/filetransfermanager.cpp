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

#include "filetransfermanager.h"
#include <QtDebug>
#include <tox/tox.h>
#include "toxaccount.h"
#include "toxthread.h"
#include "filetransferin.h"
#include "filetransferout.h"
#include "toxcontact.h"
#include "util.h"
#include "callbackmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	FileTransferManager::FileTransferManager (ToxAccount *acc)
	: QObject { acc }
	, Acc_ { acc }
	{
		connect (this,
				SIGNAL (requested (int32_t, QByteArray, uint32_t, uint64_t, QString)),
				this,
				SLOT (handleRequest (int32_t, QByteArray, uint32_t, uint64_t, QString)));
	}

	bool FileTransferManager::IsAvailable () const
	{
		return !ToxThread_.expired ();
	}

	QObject* FileTransferManager::SendFile (const QString& id,
			const QString&, const QString& name, const QString&)
	{
		const auto toxThread = ToxThread_.lock ();
		if (!toxThread)
		{
			qWarning () << Q_FUNC_INFO
					<< "Tox thread is not available";
			return nullptr;
		}

		const auto contact = Acc_->GetByAzothId (id);
		if (!contact)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to find contact by the ID"
					<< id;
			return nullptr;
		}

		const auto transfer = new FileTransferOut { id, contact->GetPubKey (), name, toxThread };
		connect (this,
				&FileTransferManager::gotFileControl,
				transfer,
				&FileTransferOut::HandleFileControl);
		connect (this,
				&FileTransferManager::gotChunkRequest,
				transfer,
				&FileTransferOut::HandleChunkRequested);
		return transfer;
	}

	void FileTransferManager::handleToxThreadChanged (const std::shared_ptr<ToxThread>& thread)
	{
		ToxThread_ = thread;
		if (!thread)
			return;

		const auto cbMgr = thread->GetCallbackManager ();
		cbMgr->Register<tox_callback_file_recv_control> (this,
				[] (FileTransferManager *pThis, uint32_t friendNum, uint32_t fileNum, TOX_FILE_CONTROL ctrl)
					{ pThis->gotFileControl (friendNum, fileNum, ctrl); });
		cbMgr->Register<tox_callback_file_recv> (this,
				[] (FileTransferManager *pThis,
						uint32_t friendNum,
						uint32_t filenum, uint32_t kind, uint64_t filesize,
						const uint8_t *rawFilename, size_t filenameLength)
				{
					const auto thread = pThis->ToxThread_.lock ();
					if (!thread)
					{
						qWarning () << Q_FUNC_INFO
								<< "thread is dead";
						return;
					}

					const auto filenameStr = reinterpret_cast<const char*> (rawFilename);
					const auto name = QString::fromUtf8 (filenameStr, filenameLength);
					Util::Sequence (pThis, thread->GetFriendPubkey (friendNum)) >>
							[=] (const QByteArray& id) { pThis->requested (friendNum, id, filenum, filesize, name); };
				});
		// TODO handle position and conditions.
		cbMgr->Register<tox_callback_file_recv_chunk> (this,
				[] (FileTransferManager *pThis,
						uint32_t friendNum, uint32_t fileNum, uint64_t position,
						const uint8_t *rawData, size_t rawSize)
				{
					const QByteArray data
					{
						reinterpret_cast<const char*> (rawData),
						static_cast<int> (rawSize)
					};
					pThis->gotData (friendNum, fileNum, data);
				});
		cbMgr->Register<tox_callback_file_chunk_request> (this, &FileTransferManager::gotChunkRequest);
	}

	void FileTransferManager::handleRequest (int32_t friendNum,
			const QByteArray& pkey, uint32_t filenum, uint64_t size, const QString& name)
	{
		const auto toxThread = ToxThread_.lock ();
		if (!toxThread)
		{
			qWarning () << Q_FUNC_INFO
					<< "Tox thread is not available";
			return;
		}

		const auto entry = Acc_->GetByPubkey (pkey);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to find entry for pubkey"
					<< pkey;
			return;
		}

		const auto transfer = new FileTransferIn
		{
			entry->GetEntryID (),
			pkey,
			friendNum,
			filenum,
			static_cast<qint64> (size),
			name,
			toxThread
		};

		connect (this,
				&FileTransferManager::gotFileControl,
				transfer,
				&FileTransferIn::HandleFileControl);
		connect (this,
				&FileTransferManager::gotData,
				transfer,
				&FileTransferIn::HandleData);

		emit fileOffered (transfer);
	}
}
}
}
