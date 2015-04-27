# Object Rename Tracking Extension

Provides an event trigger - `track_renames` - that executes the function named in the GUC `track_renames.function`.


```sql
CREATE EXTENSION track_renames;

CREATE EVENT TRIGGER track_renames ON ddl_command_end WHEN TAG IN ('ALTER TABLE', 'ALTER FUNCTION', 'ALTER TYPE', 'ALTER VIEW') EXECUTE PROCEDURE track_renames();

ALTER DATABASE examples SET track_renames.function TO log_rename;

CREATE TABLE rename_log (objtype text, objname text, subname text, newname text);

CREATE FUNCTION log_rename(objtype text, objname text, subname text, newname text) RETURNS VOID LANGUAGE sql AS $$
    INSERT INTO rename_log(objtype, objname, subname, newname) VALUES (objtype, objname, subname, newname);
$$;

CREATE TABLE test ();
ALTER TABLE test RENAME TO something_else;

SELECT * FROM rename_log;
```

