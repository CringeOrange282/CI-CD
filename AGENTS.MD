lets code the service which listen his address, with json like:
```json
{
    "user":"username",
    "operation":"operation_type1",
    "information_id":"Some kind of key",
    "information_from_date":"date"
}
```

service start processing erquest


middleware:
- checking, has every body else wait the ideticcaly request (json information equal withot user field), or result of request in cash
    - if result in cash user gets responce with result
    - if some user waits equal request, requester waits with athers
    - else request going down

process part:
Data from json created in object (or still json),
Requast a service which find data from fieds
```json
"information_id":"Some kind of key",
"information_from_date":"date"
```

If service didnt find data, response with error
Else processor gets data from service and repack json in another form

```json
{
    "operation_id":id
    "operation":"operation_type1",
    "process_data":""
}
```

And User ands in wait group with id of operation
Then operation sends to queue

Service send response like: "OK, your request in queue"

Next step is module who gives endpoint information from queue
