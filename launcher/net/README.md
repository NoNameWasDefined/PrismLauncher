# `net/` — HTTP

The launcher has one `QNetworkAccessManager` (`APPLICATION->network()`, proxy-aware). All downloads/uploads go
through `NetJob`, a `Task` that runs several requests in parallel with progress, retries and abort.

```
NetJob (ConcurrentTask) ── runs ──► NetRequest (Task) = one HTTP request
                                      ├─ Download  (GET)  ─► Sink: ByteArraySink | FileSink | MetaCacheSink
                                      ├─ Upload    (POST)
                                      ├─ Validators (ChecksumValidator...) check data while it streams
                                      └─ HeaderProxies add headers (ApiHeaderProxy: API keys + user agent)
```

| File | Role |
|---|---|
| `NetJob.h` | Container task; usage example in its class comment. |
| `NetRequest.h` | Base for one request (reply handling, redirects, errors). |
| `Download.h` / `ApiDownload.h` | `makeByteArray` (to memory), `makeFile` (to a path), `makeCached` (through the HTTP cache). `ApiDownload` adds platform API headers. |
| `Upload.h` / `ApiUpload.h` | POST requests. |
| `HttpMetaCache.h` | On-disk cache with ETag/MD5/age per entry; `APPLICATION->metacache()->resolveEntry("general", "path")`. |
| `Sink.h`, `FileSink.h`, `ByteArraySink.h`, `MetaCacheSink.h` | Where the bytes go. |
| `Validator.h`, `ChecksumValidator.h` | Integrity checks. |
| `PasteUpload.h` | Upload a log to a paste service. |

## Example

```cpp
auto output = std::make_shared<QByteArray>();
m_job = makeShared<NetJob>(tr("Get manifest"), APPLICATION->network());
m_job->addNetAction(Net::Download::makeByteArray(QUrl(url), output));
connect(m_job.get(), &NetJob::succeeded, this, [this, output] {
    auto doc = QJsonDocument::fromJson(*output);
    // ...
});
connect(m_job.get(), &NetJob::failed, this, [](const QString& reason) { qWarning() << reason; });
m_job->start();
```

To get the HTTP status code of a request, keep the `Download::Ptr` and call `replyStatusCode()` after it finished.
For basic auth, set user/password on the `QUrl` or add a `Net::RawHeaderProxy` with an `Authorization` header.

`ServerCommunication.cpp` (this fork) currently uses `APPLICATION->network()` directly with a blocking `QEventLoop`;
moving it to a `NetJob` would stop the UI from freezing during sync.
