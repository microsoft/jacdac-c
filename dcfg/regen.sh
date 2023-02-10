#!/bin/sh

set -e
set -x

npx typescript-json-schema --noExtraProps --required --defaultNumberType integer \
		srvcfg.d.ts DeviceConfig --out deviceconfig.schema.json
npx typescript-json-schema --noExtraProps --required --defaultNumberType integer \
		srvcfg.d.ts ArchConfig --out archconfig.schema.json
