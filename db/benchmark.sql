CREATE TABLE IF NOT EXISTS numbers (
    gitcommit text,
    epoch integer,
    benchmark text,
    repeat text,
    measurement text,
    scale integer,
    ns real,
    primary key (gitcommit, epoch, benchmark, repeat, measurement, scale)
);
CREATE INDEX if not exists ix_numbers_gitcommit on numbers(gitcommit);
CREATE INDEX if not exists ix_numbers_benchmark on numbers(benchmark);
CREATE INDEX if not exists ix_numbers_measurement on numbers(measurement);
CREATE INDEX if not exists ix_numbers_scale on numbers(scale);
DROP VIEW IF EXISTS summary_stats;
CREATE VIEW summary_stats as
select
    gitcommit,
    benchmark,
    measurement,
    scale,
    avg(ns) as mean,
    sqrt(avg((ns - sub.mu) * (ns - sub.mu))) as stddev,
    min(ns) as min,
    max(ns) as max
from
    numbers,
    (select avg(ns) as mu from numbers) as sub
group by gitcommit, benchmark, measurement, scale;
