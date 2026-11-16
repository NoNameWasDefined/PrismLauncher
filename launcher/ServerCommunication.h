// File overview: (This fork) Client for the modpack sync server: builds a manifest of an instance (loaders + mods) and
// uploads/downloads it. See the detailed comments inside.

#pragma once

#include "BaseInstance.h"

/*
 * Modpack sync server client (added by this fork, not part of upstream Prism Launcher).
 *
 * Idea: a server stores, for every modpack (instance name), a manifest describing its mod loaders and mods.
 * The launcher uploads that manifest when an instance is created and checks it before each launch, so that
 * everyone playing the pack has the same mods.
 *
 * Where it is called from:
 *   - InstanceCreationTask::executeTask()  -> PutManifest()   after a new instance is created
 *   - Application::launch()                -> SyncModpack()   before every launch, if "AutoSyncModpack" is on
 *
 * Settings used (registered in Application.cpp, stored in prismlauncher.cfg):
 *   ModpackSyncServerURL   base URL, e.g. "https://sync.example.org"
 *   ServerId / ServerPsswd HTTP basic-auth user and password
 *
 * HTTP API (as used here):
 *   PUT  <URL>/modpacks/<instance name>/manifest.json   body = manifest JSON  -> upload/replace the manifest
 *   GET  <URL>/modpacks/<instance name>/manifest.json                         -> read it back
 *   Status codes handled by SyncModpack(): 204 = no update, 210 = updating, 300 = manifest returned in body,
 *   anything else = error (logged).
 *
 * Manifest format (CreateManifest()):
 *   {
 *     "instance_name":  "My Pack",
 *     "loadersVersion": [ { "uid": "net.minecraft", "version": "1.20.1", ... }, ... ],  // mmc-pack.json components
 *     "mods": [ { "name": "...", "version": "...", "url": "https://..." }, ... ]         // from mods/.index/<slug>.pw.toml
 *   }
 *
 * All functions are static and BLOCKING: they wait for the HTTP reply with a nested QEventLoop on the
 * calling (UI) thread. That keeps the code simple but freezes the window while waiting, and a server that
 * doesn't answer blocks until Qt's network timeout. The non-blocking alternative is a NetJob (net/NetJob.h).
 */
class ServerUtils {  // Communication with sync server
   public:

    // Builds the manifest JSON (compact) for `instance` from its mmc-pack.json and the packwiz metadata files in
    // <mods folder>/.index. Returns an EMPTY QByteArray when the instance has no mods/.index folder
    // (e.g. a vanilla instance, or mods added by hand without platform metadata).
    static QByteArray CreateManifest(InstancePtr instance);

    // Uploads CreateManifest(instance) with an HTTP PUT. Returns the HTTP status code (0 if the request
    // failed before getting a reply, e.g. server unreachable or empty URL).
    static int PutManifest(InstancePtr instance);

    // Fetches the server's manifest for `instance` and logs what the server said.
    // NOTE: currently always returns 0 (nothing is applied locally yet), and Application::launch() treats 0
    // as failure. Decide on a convention (e.g. return true/1 on success) and update the caller.
    static int SyncModpack(InstancePtr instance);

    // HTTP GET of the manifest. Returns {status code, response body}.
    static std::pair<int, QByteArray> GetManifest(InstancePtr instance);
};
