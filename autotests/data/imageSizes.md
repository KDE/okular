# Test for specifying image sizes in markdown

(c)

## small image

* no explicit size:

![potato](potato.jpg)

* only width specified `=100x`:

![potato](potato.jpg =100x)

* only height specified `=x100`:

![potato](potato.jpg =x100)

* both specified `=100x100`:

![potato](potato.jpg =100x100)

* only width, using html `4200x`:

<img src="potato.jpg" alt="potato" width="4200"/>

* both specified, using html `4200x4200`:

<img src="potato.jpg" alt="potato" width="4200" height="4200"/>

## wide image

* no explicit size:

![1500x300](1500x300.png)

* only width specified `=100x`:

![1500x300](1500x300.png =100x)

* only height specified `=x100`:

![1500x300](1500x300.png =x100)

* both specified `=100x100`:

![1500x300](1500x300.png =100x100)

* only height specified, using html `x4200`:

<img src="1500x300.png" alt="1500x300" height="4200"/>

* both specified, using html `4200x4200`:

<img src="1500x300.png" alt="1500x300" width="4200" height="4200"/>

## tall image

* no explicit size:

![300x1500](300x1500.png)

* only width specified `=100x`:

![300x1500](300x1500.png =100x)

* only height specified `=x100`:

![300x1500](300x1500.png =x100)

* both specified `=100x100`:

![300x1500](300x1500.png =100x100)

* both specified, using html `4200x4200`:

<img src="300x1500.png" alt="300x1500" width="4200" height="4200"/>
