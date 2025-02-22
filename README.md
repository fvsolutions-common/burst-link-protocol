# Nanobind example project

Requirements
* Generate pyi bindings [ok]
* Auto build and install [ok]
* Create wheel with simplicity 
* Debugging in python anc C [ok]
* Simple to install tooling [ok]
* Auto rebuild [ok]



# Modes of installation

Simple installation
```sh
pip install -e .
```

Fast build
```sh
pip install --no-build-isolation -ve .
```

Auto rebuild on run
```sh
pip install --no-build-isolation -Ceditable.rebuild=true -ve .
```


# Stub files
They are generated automatically buy can also be generated manually
```
python -m nanobind.stubgen -m nanobind_example_ext
```

# Deployment
```
pip wheel .
```



