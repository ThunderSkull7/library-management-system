# SQLite Backup and Recovery

The application uses `data/library.db` by default. Keep backups outside the repository's ignored `data/` directory, and protect them like the live database: they contain member contact details and circulation history.

## Create a consistent backup

Install the SQLite command-line tools if `sqlite3` is unavailable. From the repository root, run this in PowerShell:

```powershell
New-Item -ItemType Directory -Force backups | Out-Null
sqlite3 data/library.db ".backup backups/library-2026-11-19.db"
sqlite3 backups/library-2026-11-19.db "PRAGMA integrity_check;"
```

The integrity check should print `ok`. Use a unique date or timestamp in each backup name; do not overwrite the only known-good copy.

SQLite's `.backup` command uses SQLite's backup API, so it can produce a consistent snapshot while the application is open. For restore, close the application first so it cannot write to the database during recovery.

## Restore a backup

1. Close the application.
2. Preserve the current database before restoring:

   ```powershell
   Copy-Item data/library.db backups/library-before-restore.db
   ```

3. Restore the chosen backup into the live database:

   ```powershell
   sqlite3 data/library.db ".restore backups/library-2026-11-19.db"
   ```

4. Check the restored database:

   ```powershell
   sqlite3 data/library.db "PRAGMA integrity_check;"
   ```

5. Start the application and confirm that the expected catalog, members, and active loans are present.

Keep the pre-restore copy until the application has been checked. Store backup copies on a separate device or protected drive; a copy on the same disk does not protect against disk failure. Do not commit database files or backups to Git.

## Custom database path

If the application was started with a custom database path, substitute that path for `data/library.db` in every command. Use the exact path printed or configured for that deployment.

This guide describes manual recovery with the SQLite CLI. The application does not currently provide scheduled backups, encrypted backup files, or automatic restore verification.
