# goquery - a little like that j-thing, only in Go

[![build status](https://secure.travis-ci.org/PuerkitoBio/goquery.png)](http://travis-ci.org/PuerkitoBio/goquery)

[![GoDoc](https://godoc.org/github.com/PuerkitoBio/goquery?status.png)](http://godoc.org/github.com/PuerkitoBio/goquery)

goquery brings a syntax and a set of features similar to [jQuery][] to the [Go language][go]. It is based on Go's [net/html package][html] and the CSS Selector library [cascadia][]. Since the net/html parser returns nodes, and not a full-featured DOM tree, jQuery's stateful manipulation functions (like height(), css(), detach()) have been left off.

Also, because the net/html parser requires UTF-8 encoding, so does goquery: it is the caller's responsibility to ensure that the source document provides UTF-8 encoded HTML. See the [wiki][] for various options to do this.

Syntax-wise, it is as close as possible to jQuery, with the same function names when possible, and that warm and fuzzy chainable interface. jQuery being the ultra-popular library that it is, I felt that writing a similar HTML-manipulating library was better to follow its API than to start anew (in the same spirit as Go's `fmt` package), even though some of its methods are less than intuitive (looking at you, [index()][index]...).

## Installation

Please note that because of the net/html dependency, goquery requires Go1.1+.

    $ go get github.com/PuerkitoBio/goquery

(optional) To run unit tests:
    
    $ cd $GOPATH/src/github.com/PuerkitoBio/goquery
    $ go test

(optional) To run benchmarks (warning: it runs for a few minutes):

    $ cd $GOPATH/src/github.com/PuerkitoBio/goquery
    $ go test -bench=".*"

## Changelog

**Note that goquery's API is now stable, and will not break.**

*    **2015-04-20** : Add `AttrOr` helper method to return the attribute's value or a default value if absent. Thanks to [piotrkowalczuk][piotr].
*    **2015-02-04** : Add more manipulation functions - Prepend* - thanks again to [Andrew Stone][thatguystone].
*    **2014-11-28** : Add more manipulation functions - ReplaceWith*, Wrap* and Unwrap - thanks again to [Andrew Stone][thatguystone].
*    **2014-11-07** : Add manipulation functions (thanks to [Andrew Stone][thatguystone]) and `*Matcher` functions, that receive compiled cascadia selectors instead of selector strings, thus avoiding potential panics thrown by goquery via `cascadia.MustCompile` calls. This results in better performance (selectors can be compiled once and reused) and more idiomatic error handling (you can handle cascadia's compilation errors, instead of recovering from panics, which had been bugging me for a long time). Note that the actual type expected is a `Matcher` interface, that `cascadia.Selector` implements. Other matcher implementations could be used.
*    **2014-11-06** : Change import paths of net/html to golang.org/x/net/html (see https://groups.google.com/forum/#!topic/golang-nuts/eD8dh3T9yyA). Make sure to update your code to use the new import path too when you call goquery with `html.Node`s.
*    **v0.3.2** : Add `NewDocumentFromReader()` (thanks jweir) which allows creating a goquery document from an io.Reader.
*    **v0.3.1** : Add `NewDocumentFromResponse()` (thanks assassingj) which allows creating a goquery document from an http response.
*    **v0.3.0** : Add `EachWithBreak()` which allows to break out of an `Each()` loop by returning false. This function was added instead of changing the existing `Each()` to avoid breaking compatibility.
*    **v0.2.1** : Make go-getable, now that [go.net/html is Go1.0-compatible][gonet] (thanks to @matrixik for pointing this out).
*    **v0.2.0** : Add support for negative indices in Slice(). **BREAKING CHANGE** `Document.Root` is removed, `Document` is now a `Selection` itself (a selection of one, the root element, just like `Document.Root` was before). Add jQuery's Closest() method.
*    **v0.1.1** : Add benchmarks to use as baseline for refactorings, refactor Next...() and Prev...() methods to use the new html package's linked list features (Next/PrevSibling, FirstChild). Good performance boost (40+% in some cases).
*    **v0.1.0** : Initial release.

## API

goquery exposes two structs, `Document` and `Selection`, and the `Matcher` interface. Unlike jQuery, which is loaded as part of a DOM document, and thus acts on its containing document, goquery doesn't know which HTML document to act upon. So it needs to be told, and that's what the `Document` type is for. It holds the root document node as the initial Selection value to manipulate.

jQuery often has many variants for the same function (no argument, a selector string argument, a jQuery object argument, a DOM element argument, ...). Instead of exposing the same features in goquery as a single method with variadic empty interface arguments, statically-typed signatures are used following this naming convention:

*    When the jQuery equivalent can be called with no argument, it has the same name as jQuery for the no argument signature (e.g.: `Prev()`), and the version with a selector string argument is called `XxxFiltered()` (e.g.: `PrevFiltered()`)
*    When the jQuery equivalent **requires** one argument, the same name as jQuery is used for the selector string version (e.g.: `Is()`)
*    The signatures accepting a jQuery object as argument are defined in goquery as `XxxSelection()` and take a `*Selection` object as argument (e.g.: `FilterSelection()`)
*    The signatures accepting a DOM element as argument in jQuery are defined in goquery as `XxxNodes()` and take a variadic argument of type `*html.Node` (e.g.: `FilterNodes()`)
*    The signatures accepting a function as argument in jQuery are defined in goquery as `XxxFunction()` and take a function as argument (e.g.: `FilterFunction()`)
*    The goquery methods that can be called with a selector string have a corresponding version that take a `Matcher` interface and are defined as `XxxMatcher()` (e.g.: `IsMatcher()`)

The complete [godoc reference documentation can be found here][doc].

Please note that Cascadia's selectors do not necessarily match all supported selectors of jQuery (Sizzle). See the [cascadia project][cascadia] for details.

## Examples

See some tips and tricks in the [wiki][].

Taken (and adapted as if executed from outside the goquery package) from example_test.go:

```Go
package main

import (
  "fmt"
  "log"

  "github.com/PuerkitoBio/goquery"
)

func ExampleScrape() {
  doc, err := goquery.NewDocument("http://metalsucks.net") 
  if err != nil {
    log.Fatal(err)
  }

  doc.Find(".reviews-wrap article .review-rhs").Each(func(i int, s *goquery.Selection) {
    band := s.Find("h3").Text()
    title := s.Find("i").Text()
    fmt.Printf("Review %d: %s - %s\n", i, band, title)
  })
}

func main() {
  ExampleScrape()
}
```

## License

The [BSD 3-Clause license][bsd], the same as the [Go language][golic]. Cascadia's license is [here][caslic].

[jquery]: http://jquery.com/
[go]: http://golang.org/
[cascadia]: https://github.com/andybalholm/cascadia
[bsd]: http://opensource.org/licenses/BSD-3-Clause
[golic]: http://golang.org/LICENSE
[caslic]: https://github.com/andybalholm/cascadia/blob/master/LICENSE
[doc]: http://godoc.org/github.com/PuerkitoBio/goquery
[index]: http://api.jquery.com/index/
[gonet]: https://github.com/golang/net/
[html]: http://godoc.org/golang.org/x/net/html
[wiki]: https://github.com/PuerkitoBio/goquery/wiki/Tips-and-tricks
[thatguystone]: https://github.com/thatguystone
[piotr]: https://github.com/piotrkowalczuk
