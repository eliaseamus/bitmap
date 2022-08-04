DROP TABLE IF EXISTS BITMAP;

CREATE TABLE IF NOT EXISTS BITMAP (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    class TEXT NOT NULL,
    alias TEXT,
    subclass TEXT,
    i INTEGER,
    r REAL,
    b BLOB
);

INSERT INTO BITMAP (name, type, class, alias, subclass) VALUES ('EN018', 'unsigned int', 'INPUTS', 'ЧВ КВ', 'FREQ');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('LT010', 'short', 'INPUTS', 'TEMP');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('FP020', 'float', 'INPUTS', 'PRES');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('WP010', 'float', 'INPUTS', 'PRES');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('CZ026', 'bit', 'OUTPUTS', 'DISCRETE');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('CZ016', 'bit', 'OUTPUTS', 'DISCRETE');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('CHECKSUM', 'int', 'USERS', 'PROGRAM');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('MASTER1', 'bit', 'USERS', 'PROGRAM');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('MASTER2', 'bit', 'USERS', 'PROGRAM');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('TOBAKSAN', 'blob', 'USERS', 'PROGRAM');
