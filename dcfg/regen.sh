#!/bin/sh

set -e
set -x
npx typescript-json-schema --noExtraProps --required \
		srvcfg.d.ts DeviceConfig > deviceconfig-generated.schema.json
