DROP TABLE IF EXISTS BITMAP;

CREATE TABLE BITMAP (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    class TEXT NOT NULL,
    subclass TEXT,
    i INTEGER,
    r REAL
);

INSERT INTO BITMAP (name, type, class, subclass) VALUES ('EN018', 'unsigned int', 'INPUTS', 'FREQ');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('LT010', 'short',        'INPUTS', 'TEMP');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('FP020', 'float',        'INPUTS', 'PRES');
INSERT INTO BITMAP (name, type, class, subclass) VALUES ('WP010', 'float',        'INPUTS', 'PRES');
