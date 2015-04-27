/* track_renames/track_renames--0.1.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION track_renames" to load this file. \quit

-- This is a blackhole
CREATE FUNCTION track_renames()
RETURNS event_trigger
AS 'MODULE_PATHNAME'
LANGUAGE C;
