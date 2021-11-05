{
  "targets": [
    {
      "target_name": "index",
      "sources": ["src/index.cc"],
      "defines": [
        'ENC_PUBLIC_KEY="<!(echo $ENC_PUBLIC_KEY)"'
      ]
    },
  ],
}