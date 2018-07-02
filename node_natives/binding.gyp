{
  "targets": [{
    "target_name": "addon",
    "sources": [
      "../code/SoftRenderer/*.cpp"
    ],
    "include_dirs" : ["<!(node -e \"require('nan')\")","../code/SoftRenderer/"]
  }]
}
