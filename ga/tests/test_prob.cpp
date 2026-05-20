#include <catch2/catch_test_macros.hpp>

#include <problem.hpp>

TEST_CASE("Empty polygon - no cover") {

    Problem p = Problem(Polygon());

    REQUIRE(p.isValidPacking());
}


TEST_CASE("Empty packing") {


    Polygon b;
    b.push_back(Point(0,0));
    b.push_back(Point(4,0));
    b.push_back(Point(4,4));

    Problem p = Problem(b);

    REQUIRE( p.isValidPacking() );
}


TEST_CASE("Fully included") {


    Polygon b;
    b.push_back(Point(0,0));
    b.push_back(Point(4,0));
    b.push_back(Point(4,4));
    b.push_back(Point(0,4));

    Problem p = Problem(b);


    Polygon c;
    c.push_back(Point(1,1));
    c.push_back(Point(3,1));
    c.push_back(Point(3,3));
    c.push_back(Point(1,3));

    p.addToPacking(c);

    REQUIRE( p.isValidPacking() );
}


TEST_CASE("Points intersect") {

    Polygon b;
    b.push_back(Point(0,0));
    b.push_back(Point(4,0));
    b.push_back(Point(4,4));
    b.push_back(Point(0,4));

    Problem p = Problem(b);


    Polygon c;
    c.push_back(Point(0,2));
    c.push_back(Point(2,0));
    c.push_back(Point(4,2));
    c.push_back(Point(2,4));

    p.addToPacking(c);

    REQUIRE( p.isValidPacking() );
}

TEST_CASE("exact cover") {

    Polygon b;
    b.push_back(Point(0,0));
    b.push_back(Point(4,0));
    b.push_back(Point(4,4));
    b.push_back(Point(0,4));

    Problem p = Problem(b);


    Polygon c;
    c.push_back(Point(0,0));
    c.push_back(Point(4,0));
    c.push_back(Point(4,4));
    c.push_back(Point(0,4));

    p.addToPacking(c);

    REQUIRE( p.isValidPacking() );
}

TEST_CASE("cover outside") {

    Polygon b;
    b.push_back(Point(1,1));
    b.push_back(Point(3,1));
    b.push_back(Point(3,3));
    b.push_back(Point(1,3));

    Problem p = Problem(b);


    Polygon c;
    c.push_back(Point(0,0));
    c.push_back(Point(4,0));
    c.push_back(Point(4,4));
    c.push_back(Point(0,4));

    p.addToPacking(c);

    REQUIRE( !p.isValidPacking() );
}


TEST_CASE("cover partially outside - startpoint out") {

    Polygon b;
    b.push_back(Point(0,0));
    b.push_back(Point(4,0));
    b.push_back(Point(4,4));
    b.push_back(Point(0,4));

    Problem p = Problem(b);

    Polygon c;
    c.push_back(Point(-1,2));
    c.push_back(Point(2,1));
    c.push_back(Point(5,2));
    c.push_back(Point(2,3));

    p.addToPacking(c);

    REQUIRE( !p.isValidPacking() );
}

TEST_CASE("cover partially outside - startpoint in") {

    Polygon b;
    b.push_back(Point(0,0));
    b.push_back(Point(4,0));
    b.push_back(Point(4,4));
    b.push_back(Point(0,4));

    Problem p = Problem(b);

    Polygon c;
    c.push_back(Point(2,1));
    c.push_back(Point(5,2));
    c.push_back(Point(2,3));
    c.push_back(Point(-1,2));

    p.addToPacking(c);

    REQUIRE( !p.isValidPacking() );
}
