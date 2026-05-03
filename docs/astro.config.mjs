// @ts-check
import { defineConfig } from "astro/config";
import starlight from "@astrojs/starlight";
import rehypeExternalLinks from "rehype-external-links";

export default defineConfig({
  site: "https://opencitations.github.io",
  base: "/triplelite",
  integrations: [
    starlight({
      title: "TripleLite",
      social: [
        {
          icon: "github",
          label: "GitHub",
          href: "https://github.com/opencitations/triplelite",
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
            { label: "Contributing", slug: "guide/contributing" },
          ],
        },
      ],
      editLink: {
        baseUrl:
          "https://github.com/opencitations/triplelite/edit/main/docs/",
      },
    }),
  ],
  markdown: {
    rehypePlugins: [
      [rehypeExternalLinks, { target: "_blank", rel: ["noopener"] }],
    ],
  },
});
