// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#ifndef Q_MOC_RUN
#include <QByteArray>
#include <QString>
#include <archive.h>
#include <archive_entry.h>
inline QByteArray runtimeJar(const QString& path, const QByteArray& metadata)
    {
        QByteArray bytes(1024 * 1024, '\0');
        size_t used = 0;
        auto writer = archive_write_new();
        archive_write_set_format_zip(writer);
        archive_write_open_memory(writer, bytes.data(), bytes.size(), &used);
        auto entry = archive_entry_new();
        const auto name = path.toUtf8();
        archive_entry_set_pathname(entry, name.constData());
        archive_entry_set_size(entry, metadata.size());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_write_header(writer, entry);
        archive_write_data(writer, metadata.constData(), metadata.size());
        archive_entry_free(entry);
        archive_write_close(writer);
        archive_write_free(writer);
        bytes.resize(used);
        return bytes;
    }

#endif
