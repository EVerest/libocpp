# Grid Profile

This is a foundational Profile designed to act as a sort of temporal grid paper
for testing out different Profile combinations. Each charging schedule period
is exactly 1 hour apart, with the limit increasing by 1 every hour. Since it
has a Stack Level of 0, any Profile with a higher Stack Level will take
precidence over it.

```json
composite_schedule: {
    "chargingRateUnit": "W",
    "chargingSchedulePeriod": [
        {
            "limit": 1.0,
            "numberPhases": 1,
            "startPeriod": 0
        },
        {
            "limit": 2.0,
            "numberPhases": 1,
            "startPeriod": 3600
        },
        {
            "limit": 3.0,
            "numberPhases": 1,
            "startPeriod": 7200
        },
        {
            "limit": 4.0,
            "numberPhases": 1,
            "startPeriod": 10800
        },
        {
            "limit": 5.0,
            "numberPhases": 1,
            "startPeriod": 14400
        },
        {
            "limit": 6.0,
            "numberPhases": 1,
            "startPeriod": 18000
        },
        {
            "limit": 7.0,
            "numberPhases": 1,
            "startPeriod": 21600
        },
        {
            "limit": 8.0,
            "numberPhases": 1,
            "startPeriod": 25200
        },
        {
            "limit": 9.0,
            "numberPhases": 1,
            "startPeriod": 28800
        },
        {
            "limit": 10.0,
            "numberPhases": 1,
            "startPeriod": 32400
        },
        {
            "limit": 11.0,
            "numberPhases": 1,
            "startPeriod": 36000
        },
        {
            "limit": 12.0,
            "numberPhases": 1,
            "startPeriod": 39600
        },
        {
            "limit": 13.0,
            "numberPhases": 1,
            "startPeriod": 43200
        },
        {
            "limit": 14.0,
            "numberPhases": 1,
            "startPeriod": 46800
        },
        {
            "limit": 15.0,
            "numberPhases": 1,
            "startPeriod": 50400
        },
        {
            "limit": 16.0,
            "numberPhases": 1,
            "startPeriod": 54000
        },
        {
            "limit": 17.0,
            "numberPhases": 1,
            "startPeriod": 57600
        },
        {
            "limit": 18.0,
            "numberPhases": 1,
            "startPeriod": 61200
        },
        {
            "limit": 19.0,
            "numberPhases": 1,
            "startPeriod": 64800
        },
        {
            "limit": 20.0,
            "numberPhases": 1,
            "startPeriod": 68400
        },
        {
            "limit": 21.0,
            "numberPhases": 1,
            "startPeriod": 72000
        },
        {
            "limit": 22.0,
            "numberPhases": 1,
            "startPeriod": 75600
        },
        {
            "limit": 23.0,
            "numberPhases": 1,
            "startPeriod": 79200
        },
        {
            "limit": 24.0,
            "numberPhases": 1,
            "startPeriod": 82800
        }
    ],
    "duration": 86400,
    "evseId": 1,
    "scheduleStart": "2024-01-17T00:00:00.000Z"
}
```
