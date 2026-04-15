// @ts-check
import { defineConfig } from "astro/config";
import starlight from "@astrojs/starlight";
import rehypeExternalLinks from "rehype-external-links";

export default defineConfig({
  site: "https://arcangelo-massari.github.io",
  base: "/litegraph",
  integrations: [
    starlight({
      title: "LiteGraph",
      social: [
        {
          icon: "github",
          label: "GitHub",
          href: "https://github.com/arcangelo-massari/litegraph",
        },
      ],
      sidebar: [
        {
          label: "Quick start",
          slug: "",
        },
        {
          label: "Guide",
          items: [
            { label: "Data model", slug: "guide/rdfterm" },
            { label: "Reverse indexing", slug: "guide/indexing" },
            { label: "Subgraph extraction", slug: "guide/subgraph" },
            { label: "rdflib interop", slug: "guide/rdflib" },
          ],
        },
      ],
      editLink: {
        baseUrl:
          "https://github.com/arcangelo-massari/litegraph/edit/main/docs/",
      },
    }),
  ],
  markdown: {
    rehypePlugins: [
      [rehypeExternalLinks, { target: "_blank", rel: ["noopener"] }],
    ],
  },
});
