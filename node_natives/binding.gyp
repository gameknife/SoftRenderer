{
  "targets": [{
    "target_name": "addon",
    "sources": [
      "./*.cpp"
    ],
    "include_dirs" : ["<!(node -e \"require('nan')\")"]
  }]
}
