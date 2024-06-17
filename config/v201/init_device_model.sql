PRAGMA foreign_keys = ON;
CREATE TABLE IF NOT EXISTS MUTABILITY (ID INT PRIMARY KEY, MUTABILITY TEXT);
CREATE TABLE IF NOT EXISTS DATATYPE (ID INT PRIMARY KEY, DATATYPE TEXT);
CREATE TABLE IF NOT EXISTS MONITOR (ID INTEGER PRIMARY KEY, "TYPE" TEXT);
CREATE TABLE IF NOT EXISTS MONITOR_CONFIG_TYPE (ID INTEGER PRIMARY KEY, "TYPE" TEXT);
CREATE TABLE IF NOT EXISTS SEVERITY(ID INTEGER PRIMARY KEY, SEVERITY TEXT);
CREATE TABLE IF NOT EXISTS VARIABLE_ATTRIBUTE_TYPE (
  ID INTEGER PRIMARY KEY AUTOINCREMENT,
  "TYPE" TEXT
);
CREATE TABLE IF NOT EXISTS COMPONENT (
  ID INTEGER PRIMARY KEY AUTOINCREMENT,
  NAME TEXT,
  INSTANCE TEXT,
  EVSE_ID INTEGER,
  CONNECTOR_ID INTEGER
);
CREATE TABLE IF NOT EXISTS VARIABLE_ATTRIBUTE (
  ID INTEGER PRIMARY KEY AUTOINCREMENT,
  VARIABLE_ID INTEGER,
  MUTABILITY_ID INTEGER,
  PERSISTENT INTEGER,
  CONSTANT INTEGER,
  TYPE_ID INTEGER,
  "VALUE" TEXT,
  FOREIGN KEY (VARIABLE_ID) REFERENCES VARIABLE (ID),
  FOREIGN KEY (TYPE_ID) REFERENCES VARIABLE_ATTRIBUTE_TYPE (ID),
  FOREIGN KEY (MUTABILITY_ID) REFERENCES MUTABILITY (ID)
);
CREATE TABLE IF NOT EXISTS VARIABLE_CHARACTERISTICS (
  ID INTEGER PRIMARY KEY AUTOINCREMENT,
  DATATYPE_ID INTEGER,
  MAX_LIMIT REAL,
  MIN_LIMIT REAL,
  SUPPORTS_MONITORING INTEGER,
  UNIT TEXT,
  VALUES_LIST TEXT,
  FOREIGN KEY (DATATYPE_ID) REFERENCES DATATYPE (ID)
);
CREATE TABLE IF NOT EXISTS VARIABLE_MONITORING (
  ID INTEGER PRIMARY KEY AUTOINCREMENT,
  VARIABLE_ID INTEGER,
  SEVERITY INTEGER,
  "TRANSACTION" INTEGER,
  TYPE_ID INTEGER,
  CONFIG_TYPE_ID INTEGER,
  "VALUE" DECIMAL,
  "REFERENCE_VALUE" TEXT,
  FOREIGN KEY (VARIABLE_ID) REFERENCES VARIABLE (ID),
  FOREIGN KEY (TYPE_ID) REFERENCES MONITOR (ID),
  FOREIGN KEY (CONFIG_TYPE_ID) REFERENCES MONITOR_CONFIG_TYPE(ID)
);
CREATE TABLE IF NOT EXISTS VARIABLE (
  ID INTEGER PRIMARY KEY AUTOINCREMENT,
  NAME TEXT,
  INSTANCE TEXT,
  COMPONENT_ID INTEGER,
  VARIABLE_CHARACTERISTICS_ID INTEGER,
  REQUIRED INTEGER DEFAULT FALSE,
  FOREIGN KEY (COMPONENT_ID) REFERENCES COMPONENT (ID),
  FOREIGN KEY (VARIABLE_CHARACTERISTICS_ID) REFERENCES VARIABLE_CHARACTERISTICS (ID)
);
BEGIN TRANSACTION;
INSERT
  OR REPLACE INTO MUTABILITY
VALUES (0, "ReadOnly");
INSERT
  OR REPLACE INTO MUTABILITY
VALUES (1, "WriteOnly");
INSERT
  OR REPLACE INTO MUTABILITY
VALUES (2, "ReadWrite");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (0, "string");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (1, "decimal");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (2, "integer");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (3, "dateTime");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (4, "boolean");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (5, "OptionList");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (6, "SequenceList");
INSERT
  OR REPLACE INTO DATATYPE
VALUES (7, "MemberList");
INSERT
  OR REPLACE INTO MONITOR
VALUES (0, "UpperThreshold");
INSERT
  OR REPLACE INTO MONITOR
VALUES (1, "LowerThreshold");
INSERT
  OR REPLACE INTO MONITOR
VALUES (2, "Delta");
INSERT
  OR REPLACE INTO MONITOR
VALUES (3, "Periodic");
INSERT
  OR REPLACE INTO MONITOR
VALUES (4, "PeriodicClockAligned");
INSERT
  OR REPLACE INTO MONITOR_CONFIG_TYPE
VALUES (0, "HardWiredMonitor");
INSERT
  OR REPLACE INTO MONITOR_CONFIG_TYPE
VALUES (1, "PreconfiguredMonitor");
INSERT
  OR REPLACE INTO MONITOR_CONFIG_TYPE
VALUES (2, "CustomMonitor");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (0, "Danger");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (1, "HardwareFailure");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (2, "SystemFailure");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (3, "Critical");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (4, "Error");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (5, "Alert");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (6, "Warning");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (7, "Notice");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (8, "Informational");
INSERT
  OR REPLACE INTO SEVERITY
VALUES (9, "Debug");
INSERT
  OR REPLACE INTO VARIABLE_ATTRIBUTE_TYPE
VALUES (0, "Actual");
INSERT
  OR REPLACE INTO VARIABLE_ATTRIBUTE_TYPE
VALUES (1, "Target");
INSERT
  OR REPLACE INTO VARIABLE_ATTRIBUTE_TYPE
VALUES (2, "MinSet");
INSERT
  OR REPLACE INTO VARIABLE_ATTRIBUTE_TYPE
VALUES (3, "MaxSet");
COMMIT;