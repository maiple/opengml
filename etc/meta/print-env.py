# prints environment variables

import os

# print environment variables
for e in os.environ:
  print(f"{e}={os.environ[e]}")