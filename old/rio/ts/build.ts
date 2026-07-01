import { $ } from "bun";

// Sadly seems we need to install yonder also for access to temper-core.
await $`bun install`.cwd("../../engine/temper.out/js/rio-engine");

// Then just install and build here.
await $`bun install`;
// Shell out so we don't live on a cached version of node_modules.
await $`bun build --production index.ts --outfile dist/rio.js`;
