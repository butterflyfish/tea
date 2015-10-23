# EMF --- Extendable Makefile Framework

Automake, Cmake etc are very strong. Why I write *EMF*? Because Automake,
Cmake etc have their own language to output Makefile. In order to handle
such popular tool, user have to understand the language and Makefile. But
*EMF*, you only need to know *Makefile*, and generally, you only need to
customize config.mk.

*EMF* was extracted from [Tea](https://github.com/butterflyfish/tea).

# How to use

It's recommended to use *git subtree* to add EMF to your project.

## install

```sh
cd your_project
git remote add origin/emf https://github.com/butterflyfish/emf.git
git subtree add --prefix=make origin/emf master --squash
ln -s make/Makefile Makefile
cp config.mk.example config.mk
vim config.mk
```

Note: before adding subtree, your repository must have one commit at least.

## update

```sh
git subtree pull --prefix=make origin/emf master --squash
```
