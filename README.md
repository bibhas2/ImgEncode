# Image to Base64 img Tag Encoder
This application can get an SVG or PNG image from clipboard or file and create a base64 encoded img tag. The img tag is then copied into the clipboard.
This lets you easily add img tag to a Jupyter Notebook or web page. This way, we can share a notebook with images embedded inside.

## Working with Clipboard
Copy an SVG or PNG image to the clipboard and the run:

```
ImgEncode
```

This will create a img tag like this and copy it into the clipboard.

<img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEU..." alt="Embedded Image" />

Now you can paste it in a notebook cell.

## Working with Files
If you have existing image files then you can convert them to URL like this.

```
ImgEncode --svg file.svg

ImgEncode --png file.png
```

This will copy the URL to the clipboard and you can paste it in a notebook.
